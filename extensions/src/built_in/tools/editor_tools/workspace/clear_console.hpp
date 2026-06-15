
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
    String brief() const override { return String("Clear editor console output"); }
    String description() const override {
        return String("Clears all log content from the editor Output panel. "
                      "Aligned with the _clear_request() flow in Godot source editor/editor_log.cpp, "
                      "calls EditorLog::clear() via call(\"clear\").");
    }

    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        (void)ctx;

        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        godot::Control *base = ei->get_base_control();
        if (!base) {
            return ToolResult::err("NO_BASE", "Editor base control not available");
        }

        Array log_nodes = base->find_children("*", "EditorLog", true, false);
        if (log_nodes.size() == 0) {
            return ToolResult::err("NO_LOG", "EditorLog node not found");
        }

        Node *editor_log = Object::cast_to<Node>(log_nodes[0]);
        if (!editor_log) {
            return ToolResult::err("INVALID_LOG", "EditorLog node invalid");
        }

        editor_log->call("clear");

        Dictionary data;
        data["cleared"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
