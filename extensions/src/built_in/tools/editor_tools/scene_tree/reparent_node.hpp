#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ReparentNodeTool : public ITool {
public:
    String name() const noexcept override { return "reparent_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Change a node's parent";
    }
    String description() const override {
        return "Moves the specified node under new_parent. index=-1 appends at the end. "
               "If the new parent is a descendant of the original node, the operation is automatically detected and rejected. All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
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
            p["default"] = static_cast<int64_t>(-1);
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

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        Node *old_parent = node->get_parent();
        if (!old_parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent");
        }
        Node *new_parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, new_parent_path, new_parent)) {
            return ToolResult::err("PARENT_NOT_FOUND", err->get("message", ""));
        }
        if (new_parent == node) {
            return ToolResult::err("SELF_PARENT",
                "Cannot set a node as its own parent");
        }
        // detect descendant
        Node *cur = new_parent;
        while (cur) {
            if (cur == node) {
                return ToolResult::err("DESCENDANT_PARENT",
                    "New parent is a descendant of the original node");
            }
            cur = cur->get_parent();
        }

        if (new_parent == old_parent) {
            // Same parent: just reorder. Defer to move_node tool... but still
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
        auto *ur = begin_undo_action("MCP: Reparent (reorder)");
        if (ur) {
                ur->add_do_method(new_parent, "move_child", node, target);
                ur->add_undo_method(new_parent, "move_child", node, cur_idx);
                commit_undo_action(ur);
            } else {
                new_parent->move_child(node, target);
            }
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["old_index"] = static_cast<int64_t>(cur_idx);
            data["new_index"] = static_cast<int64_t>(target);
            data["changed"] = true;
            return ToolResult::ok(data);
        }

        int64_t old_index = node->get_index();
        int64_t target = index;
        if (target < 0) target = new_parent->get_child_count();
        if (target > new_parent->get_child_count()) target = new_parent->get_child_count();

        auto *ur = begin_undo_action("MCP: Reparent " + node->get_name());
        if (ur) {
            ur->add_do_method(old_parent, "remove_child", node);
            ur->add_do_method(new_parent, "add_child", node, true,
                              static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(new_parent, "move_child", node, target);
            ur->add_undo_method(new_parent, "remove_child", node);
            ur->add_undo_method(old_parent, "add_child", node, true,
                                static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_undo_method(old_parent, "move_child", node, old_index);
            ur->add_do_reference(node);
            ur->add_undo_reference(node);
            commit_undo_action(ur);
        } else {
            old_parent->remove_child(node);
            new_parent->add_child(node, true, godot::Node::INTERNAL_MODE_DISABLED);
            new_parent->move_child(node, target);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_parent"] = relative_path(ctx.root, old_parent);
        data["new_parent"] = relative_path(ctx.root, new_parent);
        data["old_index"] = static_cast<int64_t>(old_index);
        data["new_index"] = static_cast<int64_t>(target);
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

