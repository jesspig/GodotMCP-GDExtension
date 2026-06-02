// @tool register
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class CloseSceneTool : public ITool {
public:
    String name() const override { return "close_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Close the currently open scene"; }
    String description() const override {
        return "Closes the currently active scene tab in the Godot editor.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei->close_scene() == OK) {
            Dictionary data;
            data["closed"] = true;
            return ToolResult::ok(data);
        }
        return ToolResult::err("CLOSE_FAILED", "close_scene failed");
    }
};

} // namespace godot_mcp
