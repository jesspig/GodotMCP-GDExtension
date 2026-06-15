
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/screenshot_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class CaptureViewportTool : public ITool {
public:
    String name() const override { return "capture_viewport"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String("Capture editor 2D/3D viewport screenshot"); }
    String description() const override {
        return String("Captures the editor's 2D or 3D viewport screenshot and saves it to "
                      "res://screenshots/. Returns file path, dimensions, and format of the screenshot.");
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary props;
        Dictionary vp_type; vp_type["type"] = "string"; vp_type["description"] = "Viewport type: \"2d\" or \"3d\""; vp_type["default"] = "2d"; props["viewport_type"] = vp_type;
        Dictionary vp_idx; vp_idx["type"] = "integer"; vp_idx["description"] = "3D viewport index"; vp_idx["default"] = 0; props["viewport_index"] = vp_idx;
        Dictionary fmt; fmt["type"] = "string"; fmt["description"] = "Image format: \"png\" or \"jpg\""; fmt["default"] = "png"; props["format"] = fmt;
        Dictionary sp; sp["type"] = "string"; sp["description"] = "Save path (res://), auto-generated if empty"; sp["default"] = ""; props["save_path"] = sp;
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String vp_type = args_string(ctx.args, "viewport_type", "2d");
        int vp_index = static_cast<int>(args_int(ctx.args, "viewport_index", 0));
        String format = args_string(ctx.args, "format", "png");
        String save_path = args_string(ctx.args, "save_path", "");
        return capture_editor_viewport(vp_type, vp_index, format, save_path);
    }
};

} // namespace godot_mcp
