// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot_mcp {

class DetachScriptTool : public ITool {
public:
    String name() const override { return "detach_script"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("移除节点上的脚本");
    }
    String description() const override {
        return String::utf8("通过 set_script(Variant()) 移除节点上的脚本。"
                            "Ctrl+Z 会恢复原有脚本。"
                            "如果节点没有脚本，返回 NO_SCRIPT 错误（不视为 no-op）。");
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
        godot::Ref<godot::Script> old_script = node->get_script();
        if (old_script.is_null()) {
            return ToolResult::err("NO_SCRIPT",
                String::utf8("节点没有附加脚本: ") + node_path);
        }
        String old_path = old_script->get_path();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Detach Script"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_script", godot::Variant());
            ur->add_undo_method(node, "set_script", old_script);
            ur->commit_action();
        } else {
            node->set_script(godot::Variant());
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["removed_script"] = old_path;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
