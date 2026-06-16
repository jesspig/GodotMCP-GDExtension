// reset_setting.hpp -- Reset a project setting to its default value

#pragma once

#include "built_in/cmd_utils/tracked_settings.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ResetSettingTool : public ITool {
public:
    String name() const noexcept override { return "reset_setting"; }
    String category() const noexcept override { return "editor_tools/settings"; }
    String brief() const noexcept override {
        return "Reset a project setting to its default value";
    }
    String description() const override {
        return "Restore a project setting to the value it had before the last set_setting call. "
               "If the setting was never modified by set_setting, reports success without changes.";
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("setting_path", "string", "Full setting path to reset")
            .required({"setting_path"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "setting_path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "setting_path is required");
        }
        auto *ps = godot::ProjectSettings::get_singleton();
        if (!ps->has_setting(path)) {
            return ToolResult::err("SETTING_NOT_FOUND",
                String("Setting not found: ") + path);
        }
        Variant old_val = ps->get_setting(path);
        auto &overrides = get_setting_overrides();
        if (overrides.has(path)) {
            // Restore to the original value (snapshotted by set_setting).
            const Variant original = overrides[path];
            overrides.erase(path);
            ps->set_setting(path, original);
            Error err = ps->save();
            if (err != godot::OK) {
                return ToolResult::err("SAVE_FAILED",
                    String("Failed to save after reset (error ") + godot::itos(err) + String(")"));
            }
            Dictionary data;
            data["setting"] = path;
            data["previous_value"] = variant_to_json(old_val);
            data["reset"] = true;
            return ToolResult::ok(data);
        }
        // No prior modification by set_setting — already at original state.
        Dictionary data;
        data["setting"] = path;
        data["previous_value"] = variant_to_json(old_val);
        data["reset"] = true;
        data["note"] = "already_at_original";
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
