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
    String brief() const override { return String("Run the project default main scene"); }
    String description() const override {
        return String("Runs the project's default main scene (configured in ProjectSettings under application/run/main_scene). "
                             "Equivalent to pressing F5 (or clicking the Run Project button) in the editor. "
                             "Can be stopped via stop_project.");
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
