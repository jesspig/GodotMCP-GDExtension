// @tool register
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
    String brief() const override { return String::utf8("捕获编辑器 2D/3D 视口截图"); }
    String description() const override {
        return String::utf8("捕获 Godot 编辑器的 2D 或 3D 视口截图并保存到 res://screenshots/。"
                            "返回截图的文件路径、尺寸、格式。");
    }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary props;
        Dictionary vp_type; vp_type["type"] = "string"; vp_type["description"] = "视口类型: \"2d\" 或 \"3d\""; vp_type["default"] = "2d"; props["viewport_type"] = vp_type;
        Dictionary vp_idx; vp_idx["type"] = "integer"; vp_idx["description"] = "3D 视口索引"; vp_idx["default"] = 0; props["viewport_index"] = vp_idx;
        Dictionary fmt; fmt["type"] = "string"; fmt["description"] = "图片格式: \"png\" 或 \"jpg\""; fmt["default"] = "png"; props["format"] = fmt;
        Dictionary sp; sp["type"] = "string"; sp["description"] = "保存路径（res://），为空则自动生成"; sp["default"] = ""; props["save_path"] = sp;
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String vp_type = args_string(ctx.args, "viewport_type", "2d");
        int vp_index = (int)args_int(ctx.args, "viewport_index", 0);
        String format = args_string(ctx.args, "format", "png");
        String save_path = args_string(ctx.args, "save_path", "");
        return capture_editor_viewport(vp_type, vp_index, format, save_path);
    }
};

} // namespace godot_mcp
