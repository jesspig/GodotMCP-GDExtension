// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot_mcp {

class MakeLocalTool : public ITool {
public:
    String name() const override { return "make_local"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("将场景实例转为本地（断开外部引用）");
    }
    String description() const override {
        return String::utf8("清除节点的 scene_file_path，将该节点及其子树转换为本地节点（不再链接回原 .tscn）。"
                            "所有 ownership 自动重写到当前场景根。"
                            "等效于编辑器 Make Local 操作。仅适用于场景实例节点。"
                            "所有变更可撤销（恢复 scene_file_path 是有限支持——必须先记录旧值）。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("场景实例节点路径");
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
        String old_sfp = node->get_scene_file_path();
        if (old_sfp.is_empty()) {
            return ToolResult::err("NOT_AN_INSTANCE",
                String::utf8("该节点不是场景实例（无 scene_file_path）"));
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Make Local ") + node->get_name(),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_scene_file_path", String());
            ur->add_undo_method(node, "set_scene_file_path", old_sfp);
            ur->commit_action();
        } else {
            node->set_scene_file_path("");
        }

        // Re-assign ownership recursively (this is the second part of Make Local)
        scene_tree_utils::assign_owner_recursive(node, ctx.root);

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_scene_file_path"] = old_sfp;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
