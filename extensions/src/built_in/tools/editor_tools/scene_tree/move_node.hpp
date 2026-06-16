
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class MoveNodeTool : public ITool {
public:
    String name() const noexcept override { return "move_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Reorder a node within its parent";
    }
    String description() const override {
        return "Moves the specified node to a given index under parent_path. "
               "position can be \"up\" (move one forward), \"down\" (move one back), \"first\" (first position), \"last\" (last position), or an integer index. "
               "Out-of-range positions are automatically clamped. All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path to move")
            .prop("position", "string", "Target position: up/down/first/last or integer index")
            .prop("parent_path", "string", "New parent path (empty = keep current parent)")
            .required(Array::make("position"))
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String position = args_string(ctx.args, "position");
        String parent_path = args_string(ctx.args, "parent_path", "");

        if (position.is_empty()) {
            return ToolResult::err("MISSING_ARG", "position cannot be empty");
        }
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        Node *old_parent = node->get_parent();
        if (!old_parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent, cannot move");
        }
        Node *new_parent = old_parent;
        if (!parent_path.is_empty()) {
            if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, new_parent)) {
                return ToolResult::err("PARENT_NOT_FOUND", err->get("message", ""));
            }
        }

        int64_t cur = node->get_index();
        int64_t target = -1;
        int64_t new_parent_count = new_parent->get_child_count();

        if (position == "up") {
            target = cur - 1;
        } else if (position == "down") {
            target = cur + 1;
        } else if (position == "first") {
            target = 0;
        } else if (position == "last") {
            target = new_parent_count - (new_parent == old_parent ? 1 : 0);
        } else {
            // parse as integer
            bool is_int = position.is_valid_int();
            if (!is_int) {
                return ToolResult::err("BAD_POSITION",
                    "position must be up/down/first/last or an integer: " + position);
            }
            target = static_cast<int64_t>(position.to_int());
        }
        // clamp
        int64_t max_idx = (new_parent == old_parent ? new_parent_count - 1 : new_parent_count);
        if (target < 0) target = 0;
        if (target > max_idx) target = max_idx;

        if (new_parent == old_parent && target == cur) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["index"] = static_cast<int64_t>(cur);
            data["changed"] = false;
            return ToolResult::ok(data);
        }

        int64_t old_index = cur;
        auto *ur = begin_undo_action("MCP: Move Node");
        if (ur) {
            if (new_parent != old_parent) {
                ur->add_do_method(old_parent, "remove_child", node);
                ur->add_do_method(new_parent, "add_child", node, true,
                                  static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
                ur->add_undo_method(new_parent, "remove_child", node);
                ur->add_undo_method(old_parent, "add_child", node, true,
                                    static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
                ur->add_undo_method(old_parent, "move_child", node, old_index);
                ur->add_do_reference(node);
                ur->add_undo_reference(node);
            }
            ur->add_do_method(new_parent, "move_child", node, target);
            ur->add_undo_method(new_parent, "move_child", node, old_index);
            commit_undo_action(ur);
        } else {
            if (new_parent != old_parent) {
                old_parent->remove_child(node);
                new_parent->add_child(node, true, godot::Node::INTERNAL_MODE_DISABLED);
            }
            new_parent->move_child(node, target);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_index"] = static_cast<int64_t>(old_index);
        data["new_index"] = static_cast<int64_t>(target);
        data["new_parent"] = relative_path(ctx.root, new_parent);
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
