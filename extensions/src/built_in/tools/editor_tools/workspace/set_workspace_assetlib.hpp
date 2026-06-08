// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class SetWorkspaceAssetLibTool : public ITool {
public:
    String name() const override { return "set_workspace_assetlib"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String::utf8("打开资源库工作区"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) return ToolResult::err("NO_EDITOR", "EditorInterface 不可用");
        ei->set_main_screen_editor("AssetLib");
        Dictionary d;
        d["workspace"] = "AssetLib";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
