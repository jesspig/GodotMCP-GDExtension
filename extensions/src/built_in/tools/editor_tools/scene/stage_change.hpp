#pragma once

#include "built_in/tool_base.hpp"
#include "scene_diff/scene_shadow.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class StageSceneChangeTool : public ITool {
public:
    String name() const noexcept override { return "stage_scene_change"; }
    String category() const noexcept override { return "editor_tools/scene"; }
    String brief() const noexcept override {
        return "Snapshot the current scene into the shadow buffer for diff preview";
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        auto *ps = godot::ProjectSettings::get_singleton();
        Variant enabled = ps->get_setting("godot_mcp/shadow_scene_enabled", false);
        if (!enabled) {
            return ToolResult::err("SHADOW_DISABLED", "Shadow scene mode is disabled in project settings");
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        Node *scene_root = ei->get_edited_scene_root();
        if (!scene_root) {
            return ToolResult::err("NO_SCENE", "No scene is currently open in the editor");
        }

        Error cap_err = scene_diff::get_global_shadow().capture();
        if (cap_err != Error::OK) {
            return ToolResult::err("SNAPSHOT_FAILED", "Failed to capture scene snapshot");
        }

        Dictionary data;
        data["scene"] = scene_diff::get_global_shadow().current_scene_path();
        data["has_shadow"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
