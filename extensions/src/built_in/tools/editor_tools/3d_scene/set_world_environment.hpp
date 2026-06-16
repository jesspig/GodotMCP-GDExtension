
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/procedural_sky_material.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class SetWorldEnvironmentTool : public ITool {
public:
    String name() const noexcept override { return "set_world_environment"; }
    String category() const noexcept override { return "editor_tools/3d_scene"; }
    String brief() const noexcept override {
        return "Create or modify a WorldEnvironment with Environment settings";
    }
    String description() const override {
        return "Creates or modifies a WorldEnvironment node with Environment resource. "
               "Supports ambient light, sky, fog, and tonemap configuration.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to existing WorldEnvironment (empty = create new)";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path for new WorldEnvironment";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Ambient light color {r, g, b} (0-1 range)";
            props["ambient_color"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Ambient light energy";
            props["ambient_energy"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Enable sky";
            p["default"] = true;
            props["sky_enabled"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Sky top color {r, g, b}";
            props["sky_color"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Enable fog";
            props["fog_enabled"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Fog color {r, g, b}";
            props["fog_color"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Fog depth begin";
            props["fog_depth_begin"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Fog depth end";
            props["fog_depth_end"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Tonemap mode: linear/reinhard/filmic/aces";
            props["tonemap_mode"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String parent_path = args_string(ctx.args, "parent_path");
        bool sky_enabled = args_bool(ctx.args, "sky_enabled", true);
        String tonemap_mode = args_string(ctx.args, "tonemap_mode");

        godot::WorldEnvironment *world_env = nullptr;
        bool created = false;

        if (!node_path.is_empty()) {
            Node *found = nullptr;
            if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, found)) {
                return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
            }
            world_env = Object::cast_to<godot::WorldEnvironment>(found);
            if (!world_env) {
                return ToolResult::err("WRONG_TYPE",
                    "Node is not a WorldEnvironment: " + found->get_class());
            }
        } else {
            Node *parent = nullptr;
            if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
                parent = ctx.root;
            }

            world_env = memnew(godot::WorldEnvironment);
            if (!world_env) {
                return ToolResult::err("CREATE_FAILED", "Failed to create WorldEnvironment");
            }
            world_env->set_name("WorldEnvironment");
            created = true;

            auto *ur = begin_undo_action("MCP: Create WorldEnvironment");
            if (!ur) {
                parent->add_child(world_env, true, Node::INTERNAL_MODE_DISABLED);
                world_env->set_owner(ctx.root);
                mark_scene_dirty();
            } else {
                commit_add_child_undo(ur, "MCP: Create WorldEnvironment", parent, world_env, ctx.root);
            }
        }

        godot::Ref<godot::Environment> env = world_env->get_environment();
        if (env.is_null()) {
            env.instantiate();
            world_env->set_environment(env);
        }

        Dictionary cd = args_get_typed<Dictionary>(ctx.args, "ambient_color", Dictionary());
        if (!cd.is_empty()) {
            real_t r = static_cast<real_t>(args_float(cd, "r", 0.5));
            real_t g = static_cast<real_t>(args_float(cd, "g", 0.5));
            real_t b = static_cast<real_t>(args_float(cd, "b", 0.5));
            env->set_ambient_light_color(godot::Color(r, g, b));
        }

        double ambient_energy = args_float(ctx.args, "ambient_energy", 0.0);
        if (ambient_energy > 0.0) {
            env->set_ambient_light_energy(static_cast<real_t>(ambient_energy));
        }

        if (sky_enabled) {
            godot::Ref<godot::Sky> sky = env->get_sky();
            if (sky.is_null()) {
                sky.instantiate();
                godot::Ref<godot::ProceduralSkyMaterial> sky_mat;
                sky_mat.instantiate();
                if (sky_mat.is_valid()) {
                    sky->set_material(sky_mat);
                }
                env->set_sky(sky);
                env->set_background(godot::Environment::BG_SKY);
            }

            Dictionary sky_cd = args_get_typed<Dictionary>(ctx.args, "sky_color", Dictionary());
            if (!sky_cd.is_empty()) {
                real_t r = static_cast<real_t>(args_float(sky_cd, "r", 0.6));
                real_t g = static_cast<real_t>(args_float(sky_cd, "g", 0.6));
                real_t b = static_cast<real_t>(args_float(sky_cd, "b", 0.9));
                godot::Ref<godot::ProceduralSkyMaterial> sky_mat = sky->get_material();
                if (sky_mat.is_valid()) {
                    sky_mat->set_sky_top_color(godot::Color(r, g, b));
                }
            }
        }

        bool fog_enabled = args_bool(ctx.args, "fog_enabled", false);
        env->set_fog_enabled(fog_enabled);

        if (fog_enabled) {
            Dictionary fog_cd = args_get_typed<Dictionary>(ctx.args, "fog_color", Dictionary());
            if (!fog_cd.is_empty()) {
                real_t r = static_cast<real_t>(args_float(fog_cd, "r", 0.5));
                real_t g = static_cast<real_t>(args_float(fog_cd, "g", 0.5));
                real_t b = static_cast<real_t>(args_float(fog_cd, "b", 0.5));
                env->set_fog_light_color(godot::Color(r, g, b));
            }

            double fog_depth_begin = args_float(ctx.args, "fog_depth_begin", 0.0);
            if (fog_depth_begin > 0.0) {
                env->set_fog_depth_begin(static_cast<real_t>(fog_depth_begin));
            }
            double fog_depth_end = args_float(ctx.args, "fog_depth_end", 0.0);
            if (fog_depth_end > 0.0) {
                env->set_fog_depth_end(static_cast<real_t>(fog_depth_end));
            }
        }

        if (!tonemap_mode.is_empty()) {
            int mode = 0;
            if (tonemap_mode == "linear") {
                mode = 0;
            } else if (tonemap_mode == "reinhard") {
                mode = 1;
            } else if (tonemap_mode == "filmic") {
                mode = 2;
            } else if (tonemap_mode == "aces") {
                mode = 3;
            }
            env->set_tonemapper(static_cast<godot::Environment::ToneMapper>(mode));
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei && world_env->is_inside_tree()) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(world_env);
            }
        }

        Dictionary data;
        data["node_name"] = world_env->get_name();
        if (world_env->is_inside_tree()) {
            data["node_path"] = relative_path(ctx.root, world_env);
        }
        data["created"] = created;
        data["sky_enabled"] = sky_enabled;
        data["fog_enabled"] = fog_enabled;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
