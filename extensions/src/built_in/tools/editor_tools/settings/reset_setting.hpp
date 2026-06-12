// reset_setting.hpp -- Reset a project setting to its default value

#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ResetSettingTool : public ITool {
public:
    String name() const override { return "reset_setting"; }
    String category() const override { return "editor_tools/settings"; }
    String brief() const override {
        return "Reset a project setting to its default value";
    }
    String description() const override {
        return "Resets the specified project setting to its default value and removes it from project.godot. "
               "Equivalent to clicking Reset in the Project Settings dialog.";
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary sp;
        sp["type"] = "string";
        sp["description"] = "Full setting path to reset";
        p["setting_path"] = sp;
        s["properties"] = p;
        Array r;
        r.push_back("setting_path");
        s["required"] = r;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "setting_path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "setting_path is required");
        }
        godot::ProjectSettings *ps = godot::ProjectSettings::get_singleton();
        if (!ps->has_setting(path)) {
            return ToolResult::err("SETTING_NOT_FOUND",
                String("Setting not found: ") + path);
        }
        Variant old_val = ps->get_setting(path);
        ps->clear(path);
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
};

}  // namespace godot_mcp
