// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class SetWorkspaceTool : public ITool {
public:
    String name() const override { return "set_workspace"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String::utf8("切换编辑器工作区"); }
    String description() const override {
        return String::utf8("切换到指定工作区：2D（2D 编辑）、3D（3D 编辑）、"
                            "Script（脚本编辑）、AssetLib（资源库）。"
                            "与 Godot 源码 editor/editor_main_screen.cpp 的 select_by_name() 流程对齐，"
                            "通过 EditorInterface::set_main_screen_editor() 实现。");
    }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标工作区名称：2D / 3D / Script / AssetLib");
            props["name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("name");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name");
        if (name.is_empty()) {
            return ToolResult::err("MISSING_ARG", "name 不能为空，可选值：2D / 3D / Script / AssetLib");
        }

        String normalized = name.strip_edges().capitalize();
        static const PackedStringArray valid_names = { "2D", "3D", "Script", "AssetLib" };
        bool valid = false;
        for (int i = 0; i < valid_names.size(); i++) {
            if (normalized == valid_names[i]) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            return ToolResult::err("INVALID_ARG",
                String("无效的工作区名称 '") + name + String("'，可选值：2D / 3D / Script / AssetLib"));
        }

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface 不可用");
        }

        ei->set_main_screen_editor(normalized);

        Dictionary data;
        data["workspace"] = normalized;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
