// @tool register
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class RunProjectTool : public ITool {
public:
    String name() const override { return "run_project"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String::utf8("运行项目的默认主场景"); }
    String description() const override {
        return String::utf8("运行项目的默认主场景（在 ProjectSettings 中配置的 application/run/main_scene）。"
                             "等同于在编辑器中按 F5（或点击「运行项目」按钮）。"
                             "场景运行后可通过 stop_project 停止。");
    }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", String::utf8("EditorInterface 不可用"));
        }
        // Check main scene exists before running
        String main_scene = ProjectSettings::get_singleton()->get_setting("application/run/main_scene", "");
        if (!main_scene.is_empty() && !ResourceLoader::get_singleton()->exists(main_scene)) {
            return ToolResult::err("SCENE_FILE_MISSING",
                String::utf8("主场景文件已被删除: ") + main_scene);
        }
        ei->play_main_scene();
        Dictionary data;
        data["action"] = "play_main_scene";
        data["status"] = "started";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
