
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class RunSpecificSceneTool : public ITool {
public:
    String name() const override { return "run_specific_scene"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String("Run a specific scene file"); }
    String description() const override {
        return String("Runs a specific scene file path. The path must start with res:// and point to a .tscn or .scn file. "
                             "Equivalent to using the Run Specific Scene dialog. "
                             "Can be stopped via stop_project.");
    }
    Dictionary build_input_schema() const override {
        Dictionary p;
        {
            Dictionary d;
            d["type"] = "string";
            d["description"] = String("Scene file path, e.g. res://scenes/level1.tscn");
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
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }
        String path = args_string(ctx.args, "scene_path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_ARG", "scene_path cannot be empty");
        }
        if (!path.begins_with("res://")) {
            return ToolResult::err("INVALID_PATH", "scene_path must start with res://");
        }
        if (!godot::ResourceLoader::get_singleton()->exists(path)) {
            return ToolResult::err("SCENE_FILE_MISSING",
                "Scene file does not exist: " + path);
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

