
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetDebuggerStateTool : public ITool {
public:
    String name() const override { return "get_debugger_state"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get debugger current state"); }
    String description() const override {
        return String("Retrieves state information from the editor debugger panel, including: "
                      "error/warning count, break state, debuggable state, session activity, "
                      "and current stack frame position. "
                      "Looks up EditorDebuggerNode via the scene tree, gets ScriptEditorDebugger, "
                      "then calls get_error_count() and similar methods via call(). "
                      "Aligned with the _update_errors() flow in Godot source "
                      "editor/debugger/editor_debugger_node.cpp.");
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

        Dictionary data;

        Object *debugger = _find_debugger_node();
        if (!debugger) {
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");
        }

        Object *active_dbg = debugger->call("get_current_debugger");
        if (active_dbg) {
            data["error_count"] = (int64_t)active_dbg->call("get_error_count");
            data["warning_count"] = (int64_t)active_dbg->call("get_warning_count");
            data["is_breaked"] = (bool)active_dbg->call("is_breaked");
            data["is_debuggable"] = (bool)active_dbg->call("is_debuggable");
            data["is_session_active"] = (bool)active_dbg->call("is_session_active");

            String stack_file = active_dbg->call("get_stack_script_file");
            data["stack_script_file"] = stack_file;
            data["stack_script_line"] = (int64_t)active_dbg->call("get_stack_script_line");
            data["stack_script_frame"] = (int64_t)active_dbg->call("get_stack_script_frame");
        } else {
            data["error_count"] = (int64_t)0;
            data["warning_count"] = (int64_t)0;
            data["is_breaked"] = false;
            data["is_debuggable"] = false;
            data["is_session_active"] = false;
        }

        Object *default_dbg = debugger->call("get_default_debugger");
        if (default_dbg) {
            data["default_session_active"] = (bool)default_dbg->call("is_session_active");
        }

        return ToolResult::ok(data);
    }

private:
    static Object *_find_debugger_node() {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) return nullptr;

        godot::Control *base = ei->get_base_control();
        if (!base) return nullptr;

        Array nodes = base->find_children("*", "EditorDebuggerNode", true, false);
        if (nodes.size() == 0) return nullptr;

        return Object::cast_to<Node>(nodes[0]);
    }
};

} // namespace godot_mcp
