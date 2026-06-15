#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class InstanceChildSceneTool : public ITool {
public:
    String name() const override { return "instance_child_scene"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Instantiate a .tscn file as a child node in the scene";
    }
    String description() const override {
        return "Instantiates a .tscn file as a child node under parent_path. "
               "instance_name left empty = uses the .tscn root node name. "
               "editable_children=true allows children to be edited in the scene. "
               "load_placeholder=true loads as a placeholder (no internal structure expanded). "
               "All changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path (empty = scene root)";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = ".tscn file res:// path to instantiate";
            props["scene_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Instance node name (empty = use .tscn root name)";
            props["instance_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Allow Editable Children on the instance";
            p["default"] = false;
            props["editable_children"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Instantiate as placeholder (don't expand internals)";
            p["default"] = false;
            props["load_placeholder"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("scene_path");
        return s;
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
        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("PARENT_NOT_FOUND",
                "Parent node not found: " + parent_path);
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
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Instance " + scene_path,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(inst, "set_scene_file_path", scene_path);
            ur->add_do_method(inst, "set_scene_instance_load_placeholder", load_placeholder);
            ur->add_do_method(parent, "add_child", inst, true,
                              static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            // set_editable_instance(child, flag) is called on the PARENT of the
            // instanced scene; passing `inst` to itself (old code) was a no-op.
            // Must run AFTER add_child so inst is a child of parent.
            if (editable_children) {
                ur->add_do_method(parent, "set_editable_instance", inst, true);
            }
            ur->add_undo_method(parent, "remove_child", inst);
            ur->add_do_reference(inst);
            ur->add_undo_reference(inst);
            ur->commit_action();

            // Also set owner outside undo for immediate consistency
            scene_tree_utils::assign_owner_recursive(inst, ctx.root);
        } else {
            inst->set_scene_file_path(scene_path);
            inst->set_scene_instance_load_placeholder(load_placeholder);
            scene_tree_utils::assign_owner_recursive(inst, ctx.root);
            parent->add_child(inst, true, godot::Node::INTERNAL_MODE_DISABLED);
            // set_editable_instance(child, flag) is called on the PARENT; the
            // old `inst->set_editable_instance(inst, true)` was a no-op.
            if (editable_children) {
                parent->set_editable_instance(inst, true);
            }
        }

        Dictionary data;
        data["instance_path"] = relative_path(ctx.root, inst);
        data["scene_path"] = scene_path;
        data["editable_children"] = editable_children;
        data["load_placeholder"] = load_placeholder;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

