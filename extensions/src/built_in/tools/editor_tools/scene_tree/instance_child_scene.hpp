#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class InstanceChildSceneTool : public ITool {
public:
    String name() const noexcept override { return "instance_child_scene"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Instantiate a .tscn file as a child node in the scene";
    }
    String description() const override {
        return "Instantiates a .tscn file as a child node under parent_path. "
               "instance_name left empty = uses the .tscn root node name. "
               "editable_children=true allows children to be edited in the scene. "
               "load_placeholder=true loads as a placeholder (no internal structure expanded). "
               "All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("parent_path", "string", "Parent node path (empty = scene root)")
            .prop("scene_path", "string", ".tscn file res:// path to instantiate")
            .prop("instance_name", "string", "Instance node name (empty = use .tscn root name)")
            .prop("editable_children", "boolean", "Allow Editable Children on the instance", false)
            .prop("load_placeholder", "boolean", "Instantiate as placeholder (don't expand internals)", false)
            .required(Array::make("scene_path"))
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path", "");
        String scene_path = args_string(ctx.args, "scene_path");
        String instance_name = args_string(ctx.args, "instance_name", "");
        bool editable_children = args_bool(ctx.args, "editable_children", false);
        bool load_placeholder = args_bool(ctx.args, "load_placeholder", false);

        if (scene_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", "scene_path cannot be empty");
        }
        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("PARENT_NOT_FOUND", err->get("message", ""));
        }

        godot::Ref<godot::PackedScene> packed =
            godot::ResourceLoader::get_singleton()->load(scene_path);
        if (packed.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                "Failed to load scene: " + scene_path);
        }
        if (!packed->can_instantiate()) {
            return ToolResult::err("NOT_INSTANTIABLE",
                "This PackedScene cannot be instantiated (may be corrupted)");
        }

        Node *inst = packed->instantiate(
            load_placeholder ? godot::PackedScene::GEN_EDIT_STATE_INSTANCE
                             : godot::PackedScene::GEN_EDIT_STATE_DISABLED);
        if (!inst) {
            return ToolResult::err("INSTANTIATE_FAILED",
                "Instantiation failed");
        }
        if (!instance_name.is_empty()) {
            inst->set_name(instance_name);
        }
        auto *ur = get_undo_redo();

        inst->set_scene_file_path(scene_path);
        inst->set_scene_instance_load_placeholder(load_placeholder);

        commit_add_child_undo(ur, "MCP: Instance " + scene_path, parent, inst, ctx.root, -1, false);

        if (editable_children) {
            parent->set_editable_instance(inst, true);
        }

        // Also set owner outside undo for immediate consistency
        scene_tree_utils::assign_owner_recursive(inst, ctx.root);

        Dictionary data;
        data["instance_path"] = relative_path(ctx.root, inst);
        data["scene_path"] = scene_path;
        data["editable_children"] = editable_children;
        data["load_placeholder"] = load_placeholder;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

