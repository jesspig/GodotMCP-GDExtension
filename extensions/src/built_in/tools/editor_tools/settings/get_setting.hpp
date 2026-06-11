// @tool register
// get_setting.hpp -- Generic project setting reader (catch-all)
// Used when the AI is unsure of the specific setting path; accepts any setting_path parameter.

#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class GetSettingTool : public ITool {
public:
    String name() const override { return "get_setting"; }
    String category() const override { return "editor_tools/settings"; }
    String brief() const override {
        return "Read any project setting by path";
    }
    String description() const override {
        return "Generic project setting reader. Accepts any setting_path parameter (e.g. \"application/config/name\"), "
               "returns the current value. Supports all built-in and custom settings.";
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        Dictionary p;
        Dictionary sp;
        sp["type"] = "string";
        sp["description"] = "Full setting path (e.g. \"application/config/name\")";
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
        ProjectSettings *ps = ProjectSettings::get_singleton();
        if (!ps->has_setting(path)) {
            return ToolResult::err("SETTING_NOT_FOUND",
                String("Setting not found: ") + path);
        }
        Variant val = ps->get_setting(path);
        Dictionary data;
        data["setting"] = path;
        data["value"] = variant_to_json(val);
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp