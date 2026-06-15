// set_setting.hpp -- Generic project setting writer (catch-all)
// Used when the AI is unsure of the specific setting path; accepts any setting_path + value parameters.

#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class SetSettingTool : public ITool {
public:
    String name() const noexcept override { return "set_setting"; }
    String category() const noexcept override { return "editor_tools/settings"; }
    String brief() const noexcept override {
        return "Set any project setting by path";
    }
    String description() const override {
        return "Generic project setting setter. Accepts any setting_path + value, "
               "modifies the setting and persists to project.godot immediately. Supports feature tag suffixes (e.g. .debug, .web).";
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary sp;
        sp["type"] = "string";
        sp["description"] = "Full setting path (e.g. \"application/config/name\")";
        p["setting_path"] = sp;
        Dictionary vp;
        vp["description"] = "Value for the setting (use native JSON types)";
        p["value"] = vp;
        s["properties"] = p;
        Array r;
        r.push_back("setting_path");
        r.push_back("value");
        s["required"] = r;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "setting_path");
        if (path.is_empty()) {
            return ToolResult::err("MISSING_PARAM", "setting_path is required");
        }
        if (!ctx.args.has("value")) {
            return ToolResult::err("MISSING_PARAM", "value is required");
        }
        auto *ps = godot::ProjectSettings::get_singleton();
        if (!ps->has_setting(path)) {
            return ToolResult::err("SETTING_NOT_FOUND",
                String("Setting not found: ") + path);
        }
        Variant old_val = ps->get_setting(path);
        Variant new_val = json_to_variant(ctx.args["value"]);
        ps->set_setting(path, new_val);
        Error err = ps->save();
        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED",
                String("Failed to save project settings (error ") + godot::itos(err) + String(")"));
        }
        Dictionary data;
        data["setting"] = path;
        data["previous_value"] = variant_to_json(old_val);
        data["new_value"] = variant_to_json(new_val);
        data["saved"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
