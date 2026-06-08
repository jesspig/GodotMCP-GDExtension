// @tool register
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
    String brief() const override { return String::utf8("获取当前调试栈追踪"); }
    String description() const override {
        return String::utf8("当调试器处于中断状态时，返回当前栈追踪信息。"
                            "包括栈帧列表（帧号、文件、函数名、行号）。"
                            "与 Godot 源码 editor/debugger/script_editor_debugger.cpp 的 "
                            "_msg_stack_dump() 流程对齐。仅在调试器中断时可用。");
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
            return ToolResult::err("NO_DEBUGGER", "未找到 EditorDebuggerNode");
        }

        Object *active_dbg = debugger->call("get_current_debugger");
        if (!active_dbg) {
            return ToolResult::err("NO_ACTIVE_DEBUGGER", "没有活跃的调试器会话");
        }

        bool is_breaked = (bool)active_dbg->call("is_breaked");
        if (!is_breaked) {
            Dictionary data;
            data["breaked"] = false;
            data["message"] = String::utf8("调试器未处于中断状态，无栈追踪数据");
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
