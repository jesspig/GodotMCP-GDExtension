// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ToggleEditableChildrenTool : public ITool {
public:
    String name() const override { return "toggle_editable_children"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("切换实例场景的\"可编辑子节点\"状态");
    }
    String description() const override {
        return String::utf8("对场景实例节点（具有 scene_file_path 的节点）切换可编辑子节点（Editable Children）状态。"
                            "enable=true 允许编辑子节点（修改不会同步回原场景）；"
                            "false 锁定为只读实例视图。"
                            "非场景实例节点会返回错误。所有变更可撤销。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("场景实例节点路径");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("true = 启用可编辑，false = 禁用");
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
        // default: toggle (use inverse of current state)
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }
        if (node->get_scene_file_path().is_empty()) {
            return ToolResult::err("NOT_AN_INSTANCE",
                String::utf8("该节点不是场景实例（无 scene_file_path）"));
        }
        bool current = node->is_editable_instance(node);
        bool enable;
        if (ctx.args.has("enable")) {
            enable = (bool)ctx.args["enable"];
        } else {
            enable = !current;
        }
        if (current == enable) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["editable_children"] = enable;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Toggle Editable Children"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_editable_instance", node, enable);
            ur->add_undo_method(node, "set_editable_instance", node, current);
            ur->commit_action();
        } else {
            node->set_editable_instance(node, enable);
        }
        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["editable_children"] = enable;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
