#pragma once

#include "built_in/tool_base.hpp"
#include "scene_diff/scene_shadow.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class DiscardChangesTool : public ITool {
public:
    String name() const noexcept override { return "discard_changes"; }
    String category() const noexcept override { return "editor_tools/scene"; }
    String brief() const noexcept override {
        return "Discard staged changes without applying them to the scene";
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        auto *ps = godot::ProjectSettings::get_singleton();
        Variant enabled = ps->get_setting("godot_mcp/shadow_scene_enabled", false);
        if (!enabled) {
            return ToolResult::err("SHADOW_DISABLED", "Shadow scene mode is disabled in project settings");
        }

        auto &shadow = scene_diff::get_global_shadow();
        if (!shadow.has_shadow()) {
            return ToolResult::err("NO_SHADOW", "Nothing to discard");
        }

        shadow.clear();

        Dictionary data;
        data["discarded"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
