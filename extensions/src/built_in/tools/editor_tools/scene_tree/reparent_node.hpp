// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ReparentNodeTool : public ITool {
public:
    String name() const override { return "reparent_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Change a node's parent";
    }
    String description() const override {
        return "Moves the specified node under new_parent. index=-1 appends at the end. "
               "If the new parent is a descendant of the original node, the operation is automatically detected and rejected. All changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path to move";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "New parent node path";
            props["new_parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Insert position in the new parent (-1 = end)";
            p["default"] = (int64_t)-1;
            props["index"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "new_parent_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String new_parent_path = args_string(ctx.args, "new_parent_path");
        int64_t index = args_int(ctx.args, "index", -1);

        if (new_parent_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", "new_parent_path cannot be empty");
        }

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        Node *old_parent = node->get_parent();
        if (!old_parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent");
        }
        Node *new_parent = resolve_node(ctx.root, new_parent_path);
        if (!new_parent) {
            return ToolResult::err("PARENT_NOT_FOUND",
                "New parent node not found: " + new_parent_path);
        }
        if (new_parent == node) {
            return ToolResult::err("SELF_PARENT",
                "Cannot set a node as its own parent"));
        }
        // detect descendant
        Node *cur = new_parent;
        while (cur) {
            if (cur == node) {
                return ToolResult::err("DESCENDANT_PARENT",
                    "New parent is a descendant of the original node"));
            }
            cur = cur->get_parent();
        }

        if (new_parent == old_parent) {
            // Same parent: just reorder. Defer to move_node tool — but still
            // allow this to function as a no-op-with-reorder for convenience.
            int64_t cur_idx = node->get_index();
            int64_t target = index;
            if (target < 0) target = new_parent->get_child_count() - 1;
            if (target == cur_idx) {
                Dictionary data;
                data["node"] = relative_path(ctx.root, node);
                data["changed"] = false;
                return ToolResult::ok(data);
            }
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Reparent (reorder)",
                                  godot::UndoRedo::MERGE_DISABLE, ctx.root);
                ur->add_do_method(new_parent, "move_child", node, target);
                ur->add_undo_method(new_parent, "move_child", node, cur_idx);
                ur->commit_action();
            } else {
                new_parent->move_child(node, target);
            }
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["old_index"] = (int64_t)cur_idx;
            data["new_index"] = (int64_t)target;
            data["changed"] = true;
            return ToolResult::ok(data);
        }

        int64_t old_index = node->get_index();
        int64_t target = index;
        if (target < 0) target = new_parent->get_child_count();
        if (target > new_parent->get_child_count()) target = new_parent->get_child_count();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Reparent " + node->get_name(),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(old_parent, "remove_child", node);
            ur->add_do_method(new_parent, "add_child", node, true,
                              (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_do_method(new_parent, "move_child", node, target);
            ur->add_undo_method(new_parent, "remove_child", node);
            ur->add_undo_method(old_parent, "add_child", node, true,
                                (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(old_parent, "move_child", node, old_index);
            ur->add_do_reference(node);
            ur->add_undo_reference(node);
            ur->commit_action();
        } else {
            old_parent->remove_child(node);
            new_parent->add_child(node, true, godot::Node::INTERNAL_MODE_DISABLED);
            new_parent->move_child(node, target);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_parent"] = relative_path(ctx.root, old_parent);
        data["new_parent"] = relative_path(ctx.root, new_parent);
        data["old_index"] = (int64_t)old_index;
        data["new_index"] = (int64_t)target;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
