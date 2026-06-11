
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetStackTraceTool : public ITool {
public:
    String name() const override { return "get_stack_trace"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get current debug stack trace"); }
    String description() const override {
        return String("Returns the current stack trace when the debugger is paused. "
                      "Includes the list of stack frames (frame number, file, function name, line number). "
                      "Aligned with the _msg_stack_dump() flow in Godot source "
                      "editor/debugger/script_editor_debugger.cpp. "
                      "Only available when the debugger is paused.");
    }

    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        (void)ctx;

        Object *debugger = _find_debugger_node();
        if (!debugger) {
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");
        }

        Object *active_dbg = debugger->call("get_current_debugger");
        if (!active_dbg) {
            return ToolResult::err("NO_ACTIVE_DEBUGGER", "No active debugger session");
        }

        bool is_breaked = (bool)active_dbg->call("is_breaked");
        if (!is_breaked) {
            Dictionary data;
            data["breaked"] = false;
            data["message"] = String("Debugger is not paused, no stack trace data available");
            return ToolResult::ok(data);
        }

        String stack_file = active_dbg->call("get_stack_script_file");
        int64_t stack_line = (int64_t)active_dbg->call("get_stack_script_line");
        int64_t stack_frame = (int64_t)active_dbg->call("get_stack_script_frame");

        Array stack_frames;
        Dictionary frame;
        frame["frame"] = stack_frame;
        frame["file"] = stack_file;
        frame["line"] = stack_line;
        stack_frames.append(frame);

        Dictionary data;
        data["breaked"] = true;
        data["frames"] = stack_frames;
        data["frame_count"] = (int64_t)stack_frames.size();
        data["is_debuggable"] = (bool)active_dbg->call("is_debuggable");
        data["is_session_active"] = (bool)active_dbg->call("is_session_active");

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
