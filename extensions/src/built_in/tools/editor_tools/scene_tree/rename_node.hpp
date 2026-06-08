// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class RenameNodeTool : public ITool {
public:
    String name() const override { return "rename_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("重命名场景中的节点");
    }
    String description() const override {
        return String::utf8("修改指定节点的名字。name 留空表示不变（无效操作）。"
                            "新名字在同一父节点下必须唯一。所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径（空 = 场景根）");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("新节点名");
            props["new_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("new_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String new_name = args_string(ctx.args, "new_name");

        if (new_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("new_name 不能为空"));
        }
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        if (node->get_name() == new_name) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["new_name"] = new_name;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        Node *parent = node->get_parent();
        if (parent) {
            Node *conflict = parent->get_node_or_null(String("./") + new_name);
            if (conflict && conflict != node) {
                return ToolResult::err("NAME_CONFLICT",
                    String::utf8("同名节点已存在: ") + new_name);
            }
        }

        String old_name = node->get_name();
        godot::Error err;
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Rename ") + old_name,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_name", new_name);
            ur->add_undo_method(node, "set_name", old_name);
            ur->commit_action();
            err = godot::OK;
        } else {
            node->set_name(new_name);
            err = godot::OK;
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_name"] = old_name;
        data["new_name"] = new_name;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
