// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetLocalsTool : public ITool {
public:
    String name() const override { return "get_locals"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get local variables of current stack frame"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String("Stack frame index (default 0)");
            p["default"] = 0;
            props["frame"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        int64_t frame = args_int(ctx.args, "frame", 0);

        Object *debugger = _find_debugger_node();
        if (!debugger)
            return ToolResult::err("NO_DEBUGGER", "EditorDebuggerNode not found");

        Object *active_dbg = debugger->call("get_current_debugger");
        if (!active_dbg) {
            Dictionary data;
            data["breaked"] = false;
            data["locals"] = Array();
            data["frame"] = frame;
            return ToolResult::ok(data);
        }

        bool is_breaked = (bool)active_dbg->call("is_breaked");
        if (!is_breaked) {
            Dictionary data;
            data["breaked"] = false;
            data["locals"] = Array();
            data["frame"] = frame;
            return ToolResult::ok(data);
        }

        Array locals;
        int64_t var_count = (int64_t)active_dbg->call("get_stack_variable_count", frame);
        for (int64_t i = 0; i < var_count; i++) {
            Dictionary var_info = active_dbg->call("get_stack_variable", frame, i);
            Dictionary entry;
            entry["name"] = var_info.get("name", "");
            entry["value"] = var_info.get("value", Variant());
            entry["type"] = var_info.get("type", "");
            locals.append(entry);
        }

        Dictionary data;
        data["breaked"] = true;
        data["locals"] = locals;
        data["frame"] = frame;
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
