
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class DeleteNodeTool : public ITool {
public:
    String name() const override { return "delete_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Delete a node and its children from the scene";
    }
    String description() const override {
        return "Deletes the specified node and all its children. Root node deletion is restricted by default. "
               "All changes go through EditorUndoRedoManager and can be restored with Ctrl+Z.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path to delete (empty = root, not allowed)";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Force delete root node (dangerous: may lose unsaved data)";
            p["default"] = false;
            props["force"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        bool force = args_bool(ctx.args, "force", false);

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        if (node == ctx.root && !force) {
            return ToolResult::err("ROOT_DELETE",
                "Cannot delete the scene root node; pass force=true to override");
        }

        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent, cannot be removed through add_child flow");
        }

        String deleted_path = relative_path(ctx.root, node);
        String deleted_type = node->get_class();
        int64_t index = node->get_index();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->remove_child(node);
            memdelete(node);
        } else {
            ur->create_action("MCP: Delete " + deleted_type,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);

            ur->add_do_method(parent, "remove_child", node);
            ur->add_undo_method(parent, "add_child", node, true,
                                (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(parent, "move_child", node, index);
            ur->add_undo_method(node, "set_owner", ctx.root);

            ur->add_do_reference(node);
            ur->add_undo_reference(node);

            ur->commit_action();
        }

        // Clear selection
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            godot::EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
            }
        }

        Dictionary data;
        data["deleted"] = deleted_path;
        data["type"] = deleted_type;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
