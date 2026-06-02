// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class PlayCurrentSceneTool : public ITool {
public:
    String name() const override { return "play_current_scene"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Play the current scene"; }
    String description() const override { return "Play the current scene"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface::get_singleton()->play_current_scene();
        Dictionary d; d["playing"] = true; return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
