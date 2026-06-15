
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateLight3DTool : public ITool {
public:
    String name() const noexcept override { return "create_light_3d"; }
    String category() const noexcept override { return "editor_tools/3d_scene"; }
    String brief() const noexcept override {
        return "Create a 3D light node (Directional/Omni/Spot)";
    }
    String description() const override {
        return "Creates a DirectionalLight3D, OmniLight3D, or SpotLight3D node "
               "with optional color, energy, shadow, and range configuration.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Light type: directional/omni/spot";
            p["default"] = "directional";
            props["light_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name (empty = auto)";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Light color {r, g, b} (0-1 range)";
            props["color"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Light energy";
            props["energy"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Enable shadows";
            props["shadow_enabled"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Light range (omni/spot only)";
            props["range"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Spot angle in degrees (spot only)";
            props["spot_angle"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("parent_path");
            s["required"] = req;
        }
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String light_type = args_string(ctx.args, "light_type", "directional").to_lower();
        String node_name = args_string(ctx.args, "node_name");
        double energy = args_float(ctx.args, "energy", 0.0);
        bool shadow = args_bool(ctx.args, "shadow_enabled", false);
        double range = args_float(ctx.args, "range", 0.0);
        double spot_angle = args_float(ctx.args, "spot_angle", 0.0);

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        static const auto kLightClasses = godot_mcp::make_dispatch_map<godot::String, godot::String>(
            std::pair{godot::String("directional"), godot::String("DirectionalLight3D")},
            std::pair{godot::String("omni"),        godot::String("OmniLight3D")},
            std::pair{godot::String("spot"),        godot::String("SpotLight3D")}
        );
        assert(kLightClasses.validate() && "Duplicate key");

        const godot::String* matched = kLightClasses.find(godot::String(light_type));
        if (!matched) {
            return ToolResult::err("INVALID_LIGHT_TYPE",
                String("Unknown light_type: ") + light_type + " (expected directional/omni/spot)");
        }
        godot::String class_name = *matched;
        godot::String default_name = class_name;

        Node *light_node = Object::cast_to<Node>(ClassDB::instantiate(class_name));
        if (!light_node) {
            return ToolResult::err("CREATE_FAILED", "Failed to create " + class_name);
        }

        if (node_name.is_empty()) {
            node_name = default_name;
        }
        light_node->set_name(node_name);

        auto *light = Object::cast_to<godot::Light3D>(light_node);
        if (!light) {
            memdelete(light_node);
            return ToolResult::err("CAST_FAILED", "Failed to cast to Light3D");
        }

        Dictionary cd = args_get_typed<Dictionary>(ctx.args, "color", Dictionary());
        if (!cd.is_empty()) {
            real_t r = static_cast<real_t>(args_float(cd, "r", 1.0));
            real_t g = static_cast<real_t>(args_float(cd, "g", 1.0));
            real_t b = static_cast<real_t>(args_float(cd, "b", 1.0));
            light->set_color(godot::Color(r, g, b));
        }

        if (energy > 0.0) {
            light->set_param(godot::Light3D::PARAM_ENERGY, static_cast<real_t>(energy));
        }

        if (shadow) {
            light->set_shadow(true);
        }

        if (range > 0.0 && light_type != "directional") {
            light->set_param(godot::Light3D::PARAM_RANGE, static_cast<real_t>(range));
        }

        if (spot_angle > 0.0 && light_type == "spot") {
            light->set_param(godot::Light3D::PARAM_SPOT_ANGLE, static_cast<real_t>(spot_angle));
        }

        auto *ur = begin_undo_action("MCP: Create " + class_name);
        if (!ur) {
            parent->add_child(light_node, true, Node::INTERNAL_MODE_DISABLED);
            light_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            commit_add_child_undo(ur, "MCP: Create " + class_name, parent, light_node, ctx.root);
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(light_node);
            }
        }

        Dictionary data;
        data["node_name"] = light_node->get_name();
        data["node_path"] = relative_path(ctx.root, light_node);
        data["class_name"] = class_name;
        data["light_type"] = light_type;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
