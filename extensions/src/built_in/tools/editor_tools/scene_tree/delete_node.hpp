// @tool register
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
        return String::utf8("从场景中删除节点（含子节点）");
    }
    String description() const override {
        return String::utf8("删除指定节点及其所有子节点。默认不能在 safe_mode 下删除根节点。"
                            "所有变更通过 EditorUndoRedoManager 提交，可通过 Ctrl+Z 恢复被删节点。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要删除的节点路径（空 = 根节点，不允许）");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("是否强制删除根节点（危险：可能丢失未保存数据）");
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
                String::utf8("节点未找到: ") + node_path);
        }
        if (node == ctx.root && !force) {
            return ToolResult::err("ROOT_DELETE",
                String::utf8("不能删除场景根节点；如确认要删除请传 force=true"));
        }

        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                String::utf8("节点没有父节点，无法通过 add_child 流程删除"));
        }

        String deleted_path = relative_path(ctx.root, node);
        String deleted_type = node->get_class();
        int64_t index = node->get_index();

        EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->remove_child(node);
            memdelete(node);
        } else {
            ur->create_action(String::utf8("MCP: Delete ") + deleted_type,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);

            ur->add_do_method(parent, "remove_child", node);
            ur->add_undo_method(parent, "add_child", node, true,
                                (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(parent, "move_child", node, index);
            ur->add_undo_method(node, "set_owner", ctx.root);

            ur->add_do_reference(node);  // 保护被删对象在 do 后存活
            ur->add_undo_reference(node);  // 保护被删对象在 undo 时可恢复

            ur->commit_action();
        }

        // 清空选择
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            EditorSelection *sel = ei->get_selection();
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
