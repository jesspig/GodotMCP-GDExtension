
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class DebuggerContinueTool : public ITool {
public:
    String name() const override { return "debugger_continue"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Continue debugger execution"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Object *dbg = find_debugger();
        if (!dbg) return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");
        dbg->call("debug_continue");
        Dictionary d;
        d["action"] = "continue";
        d["performed"] = true;
        return ToolResult::ok(d);
    }

};

} // namespace godot_mcp
