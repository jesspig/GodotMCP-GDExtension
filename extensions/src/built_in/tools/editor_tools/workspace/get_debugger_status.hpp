// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetDebuggerStatusTool : public ITool {
public:
    String name() const override { return "get_debugger_status"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String::utf8("获取调试器运行状态"); }
    String description() const override { return brief(); }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Object *dbg = _find_debugger();
        if (!dbg) return ToolResult::err("NO_DEBUGGER", "未找到 EditorDebuggerNode");

        Object *active = dbg->call("get_current_debugger");
        Dictionary d;
        if (active) {
            d["is_active"] = true;
            d["is_breaked"] = (bool)active->call("is_breaked");
            d["is_debuggable"] = (bool)active->call("is_debuggable");
            d["is_session_active"] = (bool)active->call("is_session_active");
        } else {
            d["is_active"] = false;
        }
        return ToolResult::ok(d);
    }

private:
    static Object *_find_debugger() {
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
