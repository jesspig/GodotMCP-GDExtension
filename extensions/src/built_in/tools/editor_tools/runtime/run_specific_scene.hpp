// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class RunSpecificSceneTool : public ITool {
public:
    String name() const override { return "run_specific_scene"; }
    String category() const override { return "editor_tools/runtime"; }
    String brief() const override { return String::utf8("运行指定的场景文件"); }
    String description() const override {
        return String::utf8("运行指定的场景文件路径。路径应以 res:// 开头，指向一个 .tscn 或 .scn 文件。"
                             "等同于使用「运行特定场景」对话框。"
                             "场景运行后可通过 stop_project 停止。");
    }
    Dictionary input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String::utf8("场景文件路径，如 res://scenes/level1.tscn");
            p["scene_path"] = d;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = p;
        s["required"] = Array::make("scene_path");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }
        String path = args_string(ctx.args, "scene_path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("scene_path 不能为空"));
        }
        if (!path.begins_with("res://")) {
            return ToolResult::err("INVALID_PATH", String::utf8("scene_path 必须以 res:// 开头"));
        }
        ei->play_custom_scene(path);
        Dictionary data;
        data["action"] = "play_custom_scene";
        data["scene_path"] = path;
        data["status"] = "started";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
