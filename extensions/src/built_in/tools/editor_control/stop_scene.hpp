// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class StopSceneTool : public ITool {
public:
    String name() const override { return "stop_scene"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Stop the running scene"; }
    String description() const override { return "Stop the running scene"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface::get_singleton()->call_deferred("stop_playing_scene");
        Dictionary d; d["stopped"] = true; return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
