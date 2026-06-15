
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/screenshot_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class CaptureGameViewportTool : public ITool {
public:
    String name() const noexcept override { return "capture_game_viewport"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Capture running game viewport screenshot"); }
    String description() const override {
        return String("Captures the currently running game's viewport screenshot and saves it to "
                      "res://screenshots/. Returns file path, dimensions, and format of the screenshot. "
                      "Returns an error if the game is not running.");
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary props;
        Dictionary fmt; fmt["type"] = "string"; fmt["description"] = "Image format: \"png\" or \"jpg\""; fmt["default"] = "png"; props["format"] = fmt;
        Dictionary sp; sp["type"] = "string"; sp["description"] = "Save path (res://), auto-generated if empty"; sp["default"] = ""; props["save_path"] = sp;
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String format = args_string(ctx.args, "format", "png");
        String save_path = args_string(ctx.args, "save_path", "");
        return capture_game_viewport_impl(format, save_path);
    }
};

} // namespace godot_mcp
