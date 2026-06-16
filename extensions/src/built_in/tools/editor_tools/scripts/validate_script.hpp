#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

template <ScriptLang Lang>
class ValidateScriptTool : public ITool {
    String ext() const {
        if constexpr (Lang == ScriptLang::GDScript) return ".gd";
        else return ".cs";
    }
    String lang_name() const {
        if constexpr (Lang == ScriptLang::GDScript) return "GDScript";
        else return "C#";
    }
    String lang_id() const {
        if constexpr (Lang == ScriptLang::GDScript) return "gdscript";
        else return "csharp";
    }
    String tool_name() const {
        if constexpr (Lang == ScriptLang::GDScript) return "validate_gd_script";
        else return "validate_csharp_script";
    }

public:
    String name() const noexcept override { return tool_name(); }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        if constexpr (Lang == ScriptLang::GDScript)
            return "Validate GDScript (.gd) file syntax";
        else
            return "Validate C# Script (.cs) file syntax";
    }
    String description() const override {
        if constexpr (Lang == ScriptLang::GDScript) {
            return "Validate GDScript file syntax using Godot's --check-only mode. Returns the validation result and exit code.";
        } else {
            return "Validate C# script file syntax using dotnet build --no-restore. Returns the validation result and exit code. Requires .NET SDK configured in the project.";
        }
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("path", "string", String("Target file path (must end with ") + ext() + String(")"))
            .required(Array::make("path"))
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(ext())) {
            return ToolResult::err("BAD_EXTENSION",
                String("Path must end with ") + ext());
        }
        if (!godot::FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "File does not exist: " + path);
        }

        if constexpr (Lang == ScriptLang::CSharp) {
            if (!script_utils::has_dotnet()) {
                return ToolResult::err("NO_DOTNET",
                    ".NET is not enabled, cannot validate C# script");
            }
        }

        auto *os = godot::OS::get_singleton();
        if (!os) {
            return ToolResult::err("NO_OS",
                "OS singleton not available");
        }

        auto *ps = godot::ProjectSettings::get_singleton();

        String exec_path;
        Array args_arr;

        if constexpr (Lang == ScriptLang::GDScript) {
            String godot_path = String("godot");
            String abs_path = ps ? ps->globalize_path(path) : path;
            String res_path = ps ? ps->globalize_path("res://") : String(".");

            args_arr.append("--check-only");
            args_arr.append("--script");
            args_arr.append(abs_path);
            args_arr.append("--path");
            args_arr.append(res_path);
            args_arr.append("--headless");
            args_arr.append("--quit");
            exec_path = godot_path;
        } else {
            String project_path = ps ? ps->globalize_path("res://") : String(".");

            args_arr.append("build");
            args_arr.append("--no-restore");
            args_arr.append("--no-build");
            args_arr.append("--project");
            args_arr.append(project_path);
            exec_path = String("dotnet");
        }

        int64_t exit_code = os->execute(exec_path, args_arr, Array(), false);

        Dictionary data;
        data["path"] = path;
        data["language"] = lang_id();
        data["valid"] = (exit_code == 0);
        data["exit_code"] = exit_code;
        return ToolResult::ok(data);
    }
};

using ValidateGdScriptTool = ValidateScriptTool<ScriptLang::GDScript>;
using ValidateCsharpScriptTool = ValidateScriptTool<ScriptLang::CSharp>;

} // namespace godot_mcp
