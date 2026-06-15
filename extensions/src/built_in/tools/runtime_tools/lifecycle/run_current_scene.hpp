
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot_mcp {

class RunCurrentSceneTool : public ITool {
public:
    String name() const override { return "run_current_scene"; }
    String category() const override { return "runtime_tools/lifecycle"; }
    String brief() const override { return String("Run the currently open scene in the editor"); }
    String description() const override {
        return String("Runs the currently open scene in the editor. Equivalent to pressing F6 (or clicking the Run Current Scene button) in the editor. "
                             "The scene must have a root node inheriting from Node. "
                             "Can be stopped via stop_project.");
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }
        String current_scene;
        Node *root = ei->get_edited_scene_root();
        if (root) {
            current_scene = root->get_scene_file_path();
            if (!current_scene.is_empty() && !godot::ResourceLoader::get_singleton()->exists(current_scene)) {
                return ToolResult::err("SCENE_FILE_MISSING",
                    "Scene file has been deleted: " + current_scene);
            }
        }
        ei->play_current_scene();
        Dictionary data;
        data["action"] = "play_current_scene";
        data["scene"] = current_scene;
        data["status"] = "started";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

