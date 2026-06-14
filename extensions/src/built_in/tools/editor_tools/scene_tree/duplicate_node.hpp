
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot_mcp {

class DuplicateNodeTool : public ITool {
public:
    String name() const override { return "duplicate_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Duplicate a node and its children (under the same parent)";
    }
    String description() const override {
        return "Deep copies the node subtree using Node::duplicate(DUPLICATE_USE_INSTANTIATION=15), "
               "inserting the new node as a sibling after the original (-1 = end). "
               "Optionally specify new_name for the copy (default \"<orig>_copy\"). "
               "All changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path to duplicate";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Copy name (empty = <original>_copy)";
            props["new_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Insert position (-1 = after original node)";
            p["default"] = static_cast<int64_t>(-1);
            props["index"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String new_name = args_string(ctx.args, "new_name", "");
        int64_t index = args_int(ctx.args, "index", -1);

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent");
        }

        Node *dup = node->duplicate(15);  // DUPLICATE_USE_INSTANTIATION | signals|groups|scripts
        if (!dup) {
            return ToolResult::err("DUPLICATE_FAILED",
                "Node::duplicate returned null");
        }

        if (new_name.is_empty()) {
            new_name = String(node->get_name()) + String("_copy");
        }
        dup->set_name(new_name);

        // resolve insert index: default = right after the source
        int64_t insert_idx = index;
        if (insert_idx < 0) {
            insert_idx = node->get_index() + 1;
        }
        int64_t new_parent_count = parent->get_child_count() + 1;  // includes the new one
        if (insert_idx > new_parent_count) insert_idx = new_parent_count;

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Duplicate " + node->get_name(),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "add_child", dup, true,
                              static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(parent, "move_child", dup, insert_idx);
            ur->add_undo_method(parent, "remove_child", dup);
            {
                godot::UndoRedo *undo_redo = ur->get_history_undo_redo(
                    ur->get_object_history_id(ctx.root));
                auto assign_callable = callable_mp_static(scene_tree_utils::assign_owner_recursive);
                undo_redo->add_do_method(assign_callable.bind(dup, ctx.root));
                undo_redo->add_undo_method(assign_callable.bind(dup, ctx.root));
            }
            ur->add_do_reference(dup);
            ur->add_undo_reference(dup);
            ur->commit_action();
        } else {
            parent->add_child(dup, true, godot::Node::INTERNAL_MODE_DISABLED);
            parent->move_child(dup, insert_idx);
        }

        // Also set owner outside undo for immediate consistency
        scene_tree_utils::assign_owner_recursive(dup, ctx.root);

        // select the new node
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            godot::EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(dup);
            }
        }

        Dictionary data;
        data["source"] = relative_path(ctx.root, node);
        data["new_node"] = relative_path(ctx.root, dup);
        data["new_name"] = dup->get_name();
        data["index"] = static_cast<int64_t>(insert_idx);
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
