
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class SetWorkspace2DTool : public ITool {
public:
    String name() const override { return "set_workspace_2d"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Switch to 2D workspace"); }
    String description() const override { return brief(); }

    Dictionary build_input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        ei->set_main_screen_editor("2D");
        Dictionary d;
        d["workspace"] = "2D";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
