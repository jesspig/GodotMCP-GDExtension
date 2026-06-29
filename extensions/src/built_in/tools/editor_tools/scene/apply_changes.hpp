#pragma once

#include "built_in/tool_base.hpp"
#include "scene_diff/scene_shadow.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ApplyChangesTool : public ITool {
public:
    String name() const noexcept override { return "apply_changes"; }
    String category() const noexcept override { return "editor_tools/scene"; }
    String brief() const noexcept override {
        return "Atomically apply all staged scene changes via UndoRedo";
    }
    bool needs_scene() const override { return true; }
    bool supports_undo() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        auto *ps = godot::ProjectSettings::get_singleton();
        Variant enabled = ps->get_setting("godot_mcp/shadow_scene_enabled", false);
        if (!enabled) {
            return ToolResult::err("SHADOW_DISABLED", "Shadow scene mode is disabled in project settings");
        }

        auto &shadow = scene_diff::get_global_shadow();
        if (!shadow.has_shadow()) {
            return ToolResult::err("NO_SHADOW", "Call stage_scene_change first");
        }

        auto diff = shadow.compute_diff();
        if (!diff.has_changes()) {
            shadow.clear();
            return ToolResult::err("NO_CHANGES", "No changes to apply");
        }

        int count = shadow.apply();
        if (count < 0) {
            return ToolResult::err("APPLY_FAILED", "Failed to apply changes");
        }

        Dictionary data;
        data["applied"] = count;
        data["action_name"] = "MCP: Apply scene changes";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
