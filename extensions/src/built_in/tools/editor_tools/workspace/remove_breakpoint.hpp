
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class RemoveBreakpointTool : public ITool {
public:
    String name() const override { return "remove_breakpoint"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Remove breakpoint from script line"); }
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

        Object *debugger = _find_debugger();
        if (!debugger)
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");

        debugger->call("set_breakpoint", script_path, line, false);

        Dictionary data;
        data["script_path"] = script_path;
        data["line"] = line;
        data["enabled"] = false;
        return ToolResult::ok(data);
    }

private:
    static Object *_find_debugger() {
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
