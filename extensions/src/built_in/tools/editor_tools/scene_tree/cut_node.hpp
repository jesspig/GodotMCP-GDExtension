// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class CutNodeTool : public ITool {
public:
    String name() const override { return "cut_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("剪切节点到剪贴板（同时删除原节点）");
    }
    String description() const override {
        return String::utf8("等价于 copy_node + delete_node 组合，但合并为单一 undo action。"
                            "粘贴前剪贴板持续有效；Ctrl+Z 可恢复被剪切节点。所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径");
            props["node_path"] = p;
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
        String node_path = args_string(ctx.args, "node_path");
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        if (node == ctx.root) {
            return ToolResult::err("ROOT_NOT_ALLOWED",
                String::utf8("不能剪切场景根节点"));
        }
        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                String::utf8("节点无父节点"));
        }

        // Pack first (before removal)
        Ref<PackedScene> scene = scene_tree_utils::pack_subtree(node);
        if (scene.is_null()) {
            return ToolResult::err("PACK_FAILED",
                String::utf8("打包节点失败"));
        }
        scene_tree_utils::set_clipboard(scene);

        int64_t index = node->get_index();
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Cut ") + node->get_name(),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "remove_child", node);
            ur->add_undo_method(parent, "add_child", node, true,
                                (int64_t)godot::Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(parent, "move_child", node, index);
            ur->add_do_reference(node);
            ur->add_undo_reference(node);
            ur->commit_action();
        } else {
            parent->remove_child(node);
        }

        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            EditorSelection *sel = ei->get_selection();
            if (sel) sel->clear();
        }

        Dictionary data;
        data["cut"] = relative_path(ctx.root, node);
        data["type"] = node->get_class();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
