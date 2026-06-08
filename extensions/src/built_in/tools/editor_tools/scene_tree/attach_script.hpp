// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot_mcp {

class AttachScriptTool : public ITool {
public:
    String name() const override { return "attach_script"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return String::utf8("附加 GDScript/C# 等脚本到节点");
    }
    String description() const override {
        return String::utf8("从 res:// 路径加载脚本并通过 set_script 绑定到节点。"
                            "会自动通知 EditorFileSystem 刷新以使新脚本生效。"
                            "如果节点已有脚本，会被新脚本替换（Ctrl+Z 恢复旧脚本）。"
                            "如果脚本类型与节点不匹配（如脚本继承自 Node3D 但节点是 Node2D），仍允许附加但运行时可能报错。");
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
            p["type"] = "string";
            p["description"] = String::utf8("脚本的 res:// 路径（.gd / .cs 等）");
            props["script_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "script_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String script_path = args_string(ctx.args, "script_path");
        if (script_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("script_path 不能为空"));
        }
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("节点未找到: ") + node_path);
        }

        godot::Ref<godot::Resource> res =
            godot::ResourceLoader::get_singleton()->load(script_path);
        if (res.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                String::utf8("无法加载脚本: ") + script_path);
        }
        godot::Ref<godot::Script> script = res;
        if (script.is_null()) {
            return ToolResult::err("NOT_A_SCRIPT",
                String::utf8("资源不是 Script 类型: ") + script_path);
        }

        godot::Ref<godot::Script> old_script = node->get_script();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action(String::utf8("MCP: Attach Script ") + script_path,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_script", script);
            if (old_script.is_valid()) {
                ur->add_undo_method(node, "set_script", old_script);
            } else {
                ur->add_undo_method(node, "set_script", godot::Variant());
            }
            ur->commit_action();
        } else {
            node->set_script(script);
        }

        notify_file_changed(script_path);

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["script"] = script_path;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
