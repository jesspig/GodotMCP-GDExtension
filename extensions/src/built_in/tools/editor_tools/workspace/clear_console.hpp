// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class ClearConsoleTool : public ITool {
public:
    String name() const override { return "clear_console"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String::utf8("清除编辑器控制台输出"); }
    String description() const override {
        return String::utf8("清除编辑器 Output 面板的全部日志内容。"
                            "与 Godot 源码 editor/editor_log.cpp 的 _clear_request() 流程对齐，"
                            "通过 call(\"clear\") 调用 EditorLog::clear()。");
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

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface 不可用");
        }

        Control *base = ei->get_base_control();
        if (!base) {
            return ToolResult::err("NO_BASE", "编辑器基础控件不可用");
        }

        Array log_nodes = base->find_children("*", "EditorLog", true, false);
        if (log_nodes.size() == 0) {
            return ToolResult::err("NO_LOG", "未找到 EditorLog 节点");
        }

        Node *editor_log = Object::cast_to<Node>(log_nodes[0]);
        if (!editor_log) {
            return ToolResult::err("INVALID_LOG", "EditorLog 节点无效");
        }

        editor_log->call("clear");

        Dictionary data;
        data["cleared"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
