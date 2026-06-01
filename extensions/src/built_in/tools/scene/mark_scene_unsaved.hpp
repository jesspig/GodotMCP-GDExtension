// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class MarkSceneUnsavedTool : public ITool {
public:
    String name() const override { return "mark_scene_unsaved"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Mark current scene as having unsaved changes"; }
    String description() const override {
        return "Triggers the editor's unsaved-changes indicator on the current scene tab.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface::get_singleton()->mark_scene_as_unsaved();
        Dictionary data;
        data["marked"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
