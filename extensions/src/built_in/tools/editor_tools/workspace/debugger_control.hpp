#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class DebuggerControlTool : public ITool {
public:
    String name() const override { return "debugger_control"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Control debugger: break/continue/step"); }
    String description() const override {
        return String("Controls the debugger execution flow: break, continue, step_over, step_into. "
                      "Aligned with the debug_break() / debug_continue() / debug_next() / debug_step() "
                      "flow in Godot source editor/debugger/editor_debugger_node.cpp.");
    }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Debug action: break / continue / step_over / step_into");
            p["default"] = "continue";
            props["action"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String action = args_string(ctx.args, "action", "continue");
        action = action.strip_edges().to_lower();

        Object *debugger = _find_debugger_node();
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
        } else {
            return ToolResult::err("INVALID_ARG",
                String("Invalid action '") + action + String("', valid values: break / continue / step_over / step_into"));
        }

        Object *active_dbg = debugger->call("get_current_debugger");
        Dictionary data;
        data["action"] = action;
        data["performed"] = true;
        if (active_dbg) {
            data["is_breaked"] = (bool)active_dbg->call("is_breaked");
            data["is_session_active"] = (bool)active_dbg->call("is_session_active");
        }

        return ToolResult::ok(data);
    }

private:
    static Object *_find_debugger_node() {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) return nullptr;

        Control *base = ei->get_base_control();
        if (!base) return nullptr;

        Array nodes = base->find_children("*", "EditorDebuggerNode", true, false);
        if (nodes.size() == 0) return nullptr;

        return Object::cast_to<Node>(nodes[0]);
    }
};

} // namespace godot_mcp
