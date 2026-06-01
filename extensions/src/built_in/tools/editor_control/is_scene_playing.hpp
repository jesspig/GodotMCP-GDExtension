// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class IsScenePlayingTool : public ITool {
public:
    String name() const override { return "is_scene_playing"; }
    String category() const override { return "editor_control"; }
    String brief() const override { return "Check if a scene is playing"; }
    String description() const override { return "Check if a scene is playing"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        bool playing = ei->is_playing_scene();
        Dictionary d; d["playing"] = playing;
        d["scene_path"] = playing ? Variant(ei->get_playing_scene()) : Variant();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
