
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class SetWorkspaceScriptTool : public ITool {
public:
    String name() const noexcept override { return "set_workspace_script"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Switch to Script workspace"); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        auto *ei = godot::EditorInterface::get_singleton();
        if (!ei) return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        ei->set_main_screen_editor("Script");
        Dictionary d;
        d["workspace"] = "Script";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
