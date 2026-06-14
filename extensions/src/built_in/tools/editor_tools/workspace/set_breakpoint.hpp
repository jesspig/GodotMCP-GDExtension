
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class SetBreakpointTool : public ITool {
public:
    String name() const override { return "set_breakpoint"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Set breakpoint on script line"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Script path (res:// prefix)");
            props["script_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String("Line number");
            props["line"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("script_path", "line");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String script_path = args_string(ctx.args, "script_path");
        int64_t line = args_int(ctx.args, "line");

        Object *debugger = find_debugger();
        if (!debugger)
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");

        debugger->call("set_breakpoint", script_path, line, true);

        Dictionary data;
        data["script_path"] = script_path;
        data["line"] = line;
        data["enabled"] = true;
        return ToolResult::ok(data);
    }

};

} // namespace godot_mcp
