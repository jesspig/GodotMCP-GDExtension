
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class RemoveBreakpointTool : public ITool {
public:
    String name() const noexcept override { return "remove_breakpoint"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Remove breakpoint from script line"); }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("script_path", "string", String("Script path (res:// prefix)"))
            .prop("line", "integer", String("Line number"))
            .required({"script_path", "line"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String script_path = args_string(ctx.args, "script_path");
        int64_t line = args_int(ctx.args, "line");

        Object *debugger = find_debugger();
        if (!debugger)
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");

        debugger->call("set_breakpoint", script_path, line, false);

        Dictionary data;
        data["script_path"] = script_path;
        data["line"] = line;
        data["enabled"] = false;
        return ToolResult::ok(data);
    }

};

} // namespace godot_mcp
