#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ValidateGdScriptTool : public ITool {
public:
    String name() const noexcept override { return "validate_gd_script"; }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        return "Validate GDScript (.gd) file syntax";
    }
    String description() const override {
        return "Validate GDScript file syntax using Godot's --check-only mode. Returns the validation result and exit code.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target file path (must end with .gd)";
            props["path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("path");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(".gd")) {
            return ToolResult::err("BAD_EXTENSION",
                "Path must end with .gd");
        }
        if (!godot::FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "File does not exist: " + path);
        }

        auto *os = godot::OS::get_singleton();
        if (!os) {
            return ToolResult::err("NO_OS",
                "OS singleton not available");
        }

        String godot_path = String("godot");
        auto *ps = godot::ProjectSettings::get_singleton();
        String abs_path = ps ? ps->globalize_path(path) : path;
        String res_path = ps ? ps->globalize_path("res://") : String(".");

        Array args_arr;
        args_arr.append("--check-only");
        args_arr.append("--script");
        args_arr.append(abs_path);
        args_arr.append("--path");
        args_arr.append(res_path);
        args_arr.append("--headless");
        args_arr.append("--quit");

        int64_t exit_code = os->execute(godot_path, args_arr, Array(), false);

        Dictionary data;
        data["path"] = path;
        data["language"] = String("gdscript");
        data["valid"] = (exit_code == 0);
        data["exit_code"] = exit_code;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

