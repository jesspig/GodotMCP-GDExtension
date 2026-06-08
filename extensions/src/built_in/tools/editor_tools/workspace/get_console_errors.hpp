// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class GetConsoleErrorsTool : public ITool {
public:
    String name() const override { return "get_console_errors"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String::utf8("获取编辑器控制台错误消息"); }
    String description() const override { return brief(); }

    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        RichTextLabel *rtl = _find_console_rtl();
        if (!rtl) return ToolResult::err("NO_CONSOLE", "未找到控制台");

        PackedStringArray lines = rtl->get_text().split("\n", false);
        Array errors;
        for (int64_t i = 0; i < lines.size(); i++) {
            String l = lines[i];
            if (l.contains("MCP") || l.contains("mcp") || l.contains("godot_mcp")) {
                continue;
            }
            if (l.contains("ERROR") || l.contains("error") || l.contains("Error")) {
                errors.append(l);
            }
        }
        Dictionary d;
        d["count"] = (int64_t)errors.size();
        d["errors"] = errors;
        return ToolResult::ok(d);
    }

private:
    static RichTextLabel *_find_console_rtl() {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) return nullptr;
        Control *base = ei->get_base_control();
        if (!base) return nullptr;
        Array logs = base->find_children("*", "EditorLog", true, false);
        if (logs.size() == 0) return nullptr;
        Node *log = Object::cast_to<Node>(logs[0]);
        if (!log) return nullptr;
        Array rtls = log->find_children("*", "RichTextLabel", true, false);
        if (rtls.size() == 0) return nullptr;
        return Object::cast_to<RichTextLabel>(rtls[0]);
    }
};

} // namespace godot_mcp
