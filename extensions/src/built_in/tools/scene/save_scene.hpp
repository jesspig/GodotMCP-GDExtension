// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SaveSceneTool : public ITool {
public:
    String name() const override { return "save_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Save the currently open scene"; }
    String description() const override {
        return "Saves the currently active scene. Also records a save version marker for dirty tracking.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        Node *root = ei->get_edited_scene_root();

        Array warnings;
        if (root) {
            warnings = collect_owner_warnings(root);
        }

        Error err = ei->save_scene();
        if (err != OK) return ToolResult::err("SAVE_FAILED", "save_scene failed; use save_scene_as first");

        String path;
        if (root) {
            path = root->get_scene_file_path();
            save_version_marker(root);
        }

        Dictionary data;
        data["saved"] = path;
        if (warnings.size() > 0) data["warning"] = warnings;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
