// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ToggleEditGroupTool : public ITool {
public:
    String name() const override { return "toggle_edit_group"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("切换节点的编组编辑状态（_edit_group_ meta）");
    }
    String description() const override {
        return String::utf8("enable=true 时为节点设置 _edit_group_ meta（编组编辑模式，子节点在父场景上下文中可独立编辑）；"
                            "false 时移除该 meta。"
                            "未指定 enable 时自动切换当前状态。"
                            "与 toggle_lock（_edit_lock_）是不同元数据。"
                            "所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("节点路径");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("true=启用编组编辑，false=禁用，留空=自动切换");
            props["enable"] = p;
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
        bool current = node->has_meta("_edit_group_");
        bool enable;
        if (ctx.args.has("enable")) {
            enable = (bool)ctx.args["enable"];
        } else {
            enable = !current;
        }
        if (current == enable) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["edit_group"] = enable;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Toggle Edit Group"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            if (enable) {
                ur->add_do_method(node, "set_meta", "_edit_group_",
                                  godot::Variant());
                ur->add_undo_method(node, "remove_meta", "_edit_group_");
            } else {
                ur->add_do_method(node, "remove_meta", "_edit_group_");
                ur->add_undo_method(node, "set_meta", "_edit_group_",
                                    godot::Variant());
            }
            ur->commit_action();
        } else {
            if (enable) {
                node->set_meta("_edit_group_", godot::Variant());
            } else {
                node->remove_meta("_edit_group_");
            }
        }
        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["edit_group"] = enable;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
