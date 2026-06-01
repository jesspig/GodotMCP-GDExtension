// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class PlayMainSceneTool : public ITool {
public:
    String name() const override { return "play_main_scene"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Play the main scene"; }
    String description() const override { return "Play the main scene"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        String ms = (String)ProjectSettings::get_singleton()->get_setting("application/run/main_scene");
        EditorInterface::get_singleton()->play_main_scene();
        Dictionary d; d["playing"] = true; d["main_scene"] = ms; return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
