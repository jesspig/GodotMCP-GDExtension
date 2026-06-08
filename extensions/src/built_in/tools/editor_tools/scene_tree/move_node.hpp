// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class MoveNodeTool : public ITool {
public:
    String name() const override { return "move_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("调整节点在父节点中的顺序");
    }
    String description() const override {
        return String::utf8("将指定节点移动到 parent_path 下的指定 index 位置。"
                            "position 可以是 \"up\"（向前移一位）、\"down\"（向后移一位）、\"first\"（首位）、\"last\"（末位）或整数索引。"
                            "新位置超出范围时自动夹到边界。所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要移动的节点路径");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标位置：up/down/first/last 或整数索引");
            props["position"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新父节点路径（空 = 保持当前父节点）");
            props["parent_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("position");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String position = args_string(ctx.args, "position");
        String parent_path = args_string(ctx.args, "parent_path", "");

        if (position.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("position 不能为空"));
        }
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        Node *old_parent = node->get_parent();
        if (!old_parent) {
            return ToolResult::err("ORPHAN_NODE",
                String::utf8("节点无父节点，无法移动"));
        }
        Node *new_parent = parent_path.is_empty() ? old_parent
                            : resolve_node(ctx.root, parent_path);
        if (!new_parent) {
            return ToolResult::err("PARENT_NOT_FOUND",
                String::utf8("新父节点未找到: ") + parent_path);
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
                    String::utf8("position 必须是 up/down/first/last 或整数: ") + position);
            }
            target = (int64_t)position.to_int();
        }
        // clamp
        int64_t max_idx = (new_parent == old_parent ? new_parent_count - 1 : new_parent_count);
        if (target < 0) target = 0;
        if (target > max_idx) target = max_idx;

        if (new_parent == old_parent && target == cur) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["index"] = (int64_t)cur;
            data["changed"] = false;
            return ToolResult::ok(data);
        }

        int64_t old_index = cur;
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Move Node"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            if (new_parent != old_parent) {
                ur->add_do_method(old_parent, "remove_child", node);
                ur->add_do_method(new_parent, "add_child", node, true,
                                  (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
                ur->add_undo_method(new_parent, "remove_child", node);
                ur->add_undo_method(old_parent, "add_child", node, true,
                                    (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
                ur->add_undo_method(old_parent, "move_child", node, old_index);
                ur->add_do_reference(node);
                ur->add_undo_reference(node);
            }
            ur->add_do_method(new_parent, "move_child", node, target);
            ur->add_undo_method(new_parent, "move_child", node, old_index);
            ur->commit_action();
        } else {
            if (new_parent != old_parent) {
                old_parent->remove_child(node);
                new_parent->add_child(node, true, godot::Node::INTERNAL_MODE_DISABLED);
            }
            new_parent->move_child(node, target);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_index"] = (int64_t)old_index;
        data["new_index"] = (int64_t)target;
        data["new_parent"] = relative_path(ctx.root, new_parent);
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
