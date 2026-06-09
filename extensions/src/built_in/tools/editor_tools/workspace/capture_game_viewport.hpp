// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/screenshot_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp {

class CaptureGameViewportTool : public ITool {
public:
    String name() const override { return "capture_game_viewport"; }
    String category() const override { return "editor_tools/workspace"; }
    String brief() const override { return String::utf8("捕获运行中游戏的视口截图"); }
    String description() const override {
        return String::utf8("捕获当前运行中游戏的视口截图并保存到 res://screenshots/。"
                            "返回截图的文件路径、尺寸、格式。游戏未运行时返回错误。");
    }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary props;
        Dictionary fmt; fmt["type"] = "string"; fmt["description"] = "图片格式: \"png\" 或 \"jpg\""; fmt["default"] = "png"; props["format"] = fmt;
        Dictionary sp; sp["type"] = "string"; sp["description"] = "保存路径（res://），为空则自动生成"; sp["default"] = ""; props["save_path"] = sp;
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
