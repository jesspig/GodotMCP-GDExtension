
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ValidateExportPresetsTool : public ITool {
public:
    String name() const override { return "validate_export_presets"; }
    String category() const override { return "editor_tools/export"; }
    String brief() const override {
        return "Validate export preset configuration completeness";
    }
    String description() const override {
        return "Validates export presets for completeness, checking platform, "
               "export path, features, and template installation status.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Preset name to validate (empty = validate all)";
            props["preset_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        String preset_name = args_string(ctx.args, "preset_name");

        Array presets = ei->call("get_export_presets");
        Array results;

        for (int i = 0; i < presets.size(); i++) {
            Dictionary preset = presets[i];
            String name = preset.get("name", "");

            if (!preset_name.is_empty() && name != preset_name) {
                continue;
            }

            Array warnings;
            Array errors;

            String platform = preset.get("platform", "");
            if (platform.is_empty()) {
                errors.append("Missing platform");
            }

            if (!preset.get("runnable", false)) {
                warnings.append("Preset is not runnable");
            }

            String export_path = preset.get("export_path", "");
            if (export_path.is_empty()) {
                warnings.append("No export path configured");
            }

            Array features = preset.get("features", Array());
            if (features.size() == 0) {
                warnings.append("No custom features defined");
            }

            Dictionary entry;
            entry["name"] = name;
            entry["platform"] = platform;
            entry["valid"] = errors.size() == 0;
            entry["warnings"] = warnings;
            entry["errors"] = errors;
            entry["warning_count"] = (int64_t)warnings.size();
            entry["error_count"] = (int64_t)errors.size();
            results.append(entry);
        }

        if (!preset_name.is_empty() && results.size() == 0) {
            return ToolResult::err("PRESET_NOT_FOUND",
                "Export preset not found: " + preset_name);
        }

        int total_errors = 0;
        int total_warnings = 0;
        for (int i = 0; i < results.size(); i++) {
            Dictionary r = results[i];
            total_errors += (int)r["error_count"];
            total_warnings += (int)r["warning_count"];
        }

        Dictionary data;
        data["presets"] = results;
        data["preset_count"] = (int64_t)results.size();
        data["total_errors"] = (int64_t)total_errors;
        data["total_warnings"] = (int64_t)total_warnings;
        data["all_valid"] = total_errors == 0;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
