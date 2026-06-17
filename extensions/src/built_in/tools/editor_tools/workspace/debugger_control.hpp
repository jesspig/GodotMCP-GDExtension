#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include "workspace_utils.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class DebuggerControlTool : public ITool {
public:
    String name() const noexcept override { return "debugger_control"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Control debugger: break/continue/step"); }
    String description() const override {
        return String("Controls the debugger execution flow: break, continue, step_over, step_into, step_out. "
                       "Aligned with the debug_break() / debug_continue() / debug_next() / debug_step() "
                       "flow in Godot source editor/debugger/editor_debugger_node.cpp. "
                       "Note: step_out uses debug_next() (same as step_over) since Godot's debugger "
                       "API does not expose a dedicated step_out command.");
    }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("action", "string", String("Debug action: break / continue / step_over / step_into / step_out"), "continue")
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String action = args_string(ctx.args, "action", "continue");
        action = action.strip_edges().to_lower();

        Object *debugger = find_debugger();
        if (!debugger) {
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");
        }

        if (action == "break") {
            debugger->call("debug_break");
        } else if (action == "continue") {
            debugger->call("debug_continue");
        } else if (action == "step_over") {
            debugger->call("debug_next");
        } else if (action == "step_into") {
            debugger->call("debug_step");
        } else if (action == "step_out") {
            debugger->call("debug_next");
        } else {
            return ToolResult::err("INVALID_ARG",
                String("Invalid action '") + action + String("', valid values: break / continue / step_over / step_into / step_out"));
        }

        Object *active_dbg = debugger->call("get_current_debugger");
        Dictionary data;
        data["action"] = action;
        data["performed"] = true;
        if (active_dbg) {
            data["is_breaked"] = static_cast<bool>(active_dbg->call("is_breaked"));
            data["is_session_active"] = static_cast<bool>(active_dbg->call("is_session_active"));
        }

        return ToolResult::ok(data);
    }

};

} // namespace godot_mcp
