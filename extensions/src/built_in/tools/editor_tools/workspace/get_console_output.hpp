
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class GetConsoleOutputTool : public ITool {
public:
    String name() const override { return "get_console_output"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Get editor console output"); }
    String description() const override {
        return String("Retrieves log content from the editor Output panel. Supports keyword search, "
                      "message type filtering, and MCP line exclusion. Messages are read from "
                      "EditorLog's RichTextLabel, aligned with the _rebuild_log() flow in "
                      "Godot source editor/editor_log.cpp.");
    }

    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Search keyword (only returns lines containing this keyword)");
            props["search"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Message type filter: std / error / warning / editor, leave empty for no filter");
            props["type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String("Whether to exclude MCP-related log lines (default true)");
            p["default"] = true;
            props["exclude_mcp"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String("Maximum lines to return (-1 = unlimited, default 500)");
            p["default"] = (int64_t)500;
            props["max_lines"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String search_text = args_string(ctx.args, "search", "");
        String type_filter = args_string(ctx.args, "type", "");
        bool exclude_mcp = args_bool(ctx.args, "exclude_mcp", true);
        int64_t max_lines = args_int(ctx.args, "max_lines", 500);

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        Control *base = ei->get_base_control();
        if (!base) {
            return ToolResult::err("NO_BASE", "Editor base control not available");
        }

        Array log_nodes = base->find_children("*", "EditorLog", true, false);
        if (log_nodes.size() == 0) {
            return ToolResult::err("NO_LOG", "EditorLog node not found");
        }

        Node *editor_log = Object::cast_to<Node>(log_nodes[0]);
        if (!editor_log) {
            return ToolResult::err("INVALID_LOG", "EditorLog node is invalid");
        }

        Array rtl_nodes = editor_log->find_children("*", "RichTextLabel", true, false);
        if (rtl_nodes.size() == 0) {
            return ToolResult::err("NO_RTL", "RichTextLabel child node not found");
        }

        RichTextLabel *rtl = Object::cast_to<RichTextLabel>(rtl_nodes[0]);
        if (!rtl) {
            return ToolResult::err("INVALID_RTL", "RichTextLabel node is invalid");
        }

        String full_text = rtl->get_text();

        PackedStringArray lines = full_text.split("\n", false);

        Array result_lines;
        int64_t count = 0;
        int64_t total = lines.size();

        for (int64_t i = 0; i < total; i++) {
            String line = lines[i];

            if (max_lines >= 0 && result_lines.size() >= max_lines) {
                break;
            }

            if (exclude_mcp && _is_mcp_line(line)) {
                continue;
            }

            if (!search_text.is_empty() && !line.containsn(search_text)) {
                continue;
            }

            if (!type_filter.is_empty() && !_matches_type(line, type_filter)) {
                continue;
            }

            result_lines.append(line);
            count++;
        }

        Dictionary data;
        data["total_lines"] = total;
        data["returned_lines"] = count;
        data["lines"] = result_lines;
        data["has_more"] = count >= max_lines && max_lines >= 0;

        return ToolResult::ok(data);
    }

private:
    static bool _is_mcp_line(const String &line) {
        return line.contains("MCP") || line.contains("mcp") || line.contains("godot_mcp");
    }

    static bool _matches_type(const String &line, const String &type) {
        if (type == "error") {
            return line.contains("ERROR") || line.contains("error") || line.contains("Error");
        }
        if (type == "warning") {
            return line.contains("WARNING") || line.contains("warning") || line.contains("Warning");
        }
        if (type == "editor") {
            return line.contains("EDITOR") || line.contains("Editor");
        }
        if (type == "std") {
            return true;
        }
        return true;
    }
};

} // namespace godot_mcp
