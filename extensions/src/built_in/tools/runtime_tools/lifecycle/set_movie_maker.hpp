// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SetMovieMakerTool : public ITool {
public:
    String name() const override { return "set_movie_maker"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String::utf8("启用或关闭 Movie Maker（录像）模式"); }
    String description() const override {
        return String::utf8("启用或关闭 Godot 的 Movie Maker（录像）模式。启用后运行项目时会自动录制生成视频文件。"
                             "输出路径和相关设置在 ProjectSettings 的 editor/movie_writer/* 中配置。"
                             "等同于编辑器中「项目 -> 启用 Movie Maker」菜单项。"
                             "可通过 is_movie_maker_enabled 查询当前开关状态。");
    }
    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "boolean";
            d["description"] = String::utf8("是否启用 Movie Maker 模式");
            p["enabled"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("enabled");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }
        bool enabled = args_bool(ctx.args, "enabled");
        ei->set_movie_maker_enabled(enabled);
        Dictionary data;
        data["action"] = "set_movie_maker_enabled";
        data["previous"] = !enabled;
        data["current"] = enabled;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
