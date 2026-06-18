
// get_setting.hpp -- Generic project setting reader (catch-all)
// Used when the AI is unsure of the specific setting path; accepts any setting_path parameter.

#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class GetSettingTool : public ITool {
public:
    String name() const noexcept override { return "get_setting"; }
    String category() const noexcept override { return "editor_tools/settings"; }
    String brief() const noexcept override {
        return "Read any project setting by path";
    }
    String description() const override {
        return "Generic project setting reader. Accepts any setting_path parameter (e.g. \"application/config/name\"), "
               "returns the current value. Supports all built-in and custom settings.";
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("setting_path", "string", "Full setting path (e.g. \"application/config/name\")")
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
        Variant val = ps->get_setting(path);
        Dictionary data;
        data["setting"] = path;
        data["value"] = variant_to_json(val);
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
