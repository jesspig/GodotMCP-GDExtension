
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/screenshot_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class CaptureViewportTool : public ITool {
public:
    String name() const noexcept override { return "capture_viewport"; }
    String category() const noexcept override { return "editor_tools/workspace"; }
    String brief() const noexcept override { return String("Capture editor 2D/3D viewport screenshot"); }
    String description() const override {
        return String("Captures the editor's 2D or 3D viewport screenshot and saves it to "
                      "res://screenshots/. Returns file path, dimensions, and format of the screenshot.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("viewport_type", "string", "Viewport type: \"2d\" or \"3d\"", "2d")
            .prop("viewport_index", "integer", "3D viewport index", 0)
            .prop("format", "string", "Image format: \"png\" or \"jpg\"", "png")
            .prop("save_path", "string", "Save path (res://), auto-generated if empty", "")
            .build();
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
