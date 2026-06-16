#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

namespace godot_mcp {

template <ScriptLang Lang>
class ListScriptsTool : public ITool {
    String ext() const {
        if constexpr (Lang == ScriptLang::GDScript) return ".gd";
        else return ".cs";
    }
    String lang_id() const {
        if constexpr (Lang == ScriptLang::GDScript) return "gdscript";
        else return "csharp";
    }
    String tool_name() const {
        if constexpr (Lang == ScriptLang::GDScript) return "list_gd_scripts";
        else return "list_csharp_scripts";
    }

public:
    String name() const noexcept override { return tool_name(); }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        if constexpr (Lang == ScriptLang::GDScript)
            return "List GDScript (.gd) files in the project";
        else
            return "List C# Script (.cs) files in the project";
    }
    String description() const override {
        if constexpr (Lang == ScriptLang::GDScript)
            return "Recursively traverse the project directory and list all GDScript files. Supports filtering by directory, excluding addons, and maximum results limit.";
        else
            return "Recursively traverse the project directory and list all C# script files. Supports filtering by directory, excluding addons, and maximum results limit.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("directory", "string", "Optional: search root directory (default res://)")
            .prop("include_addons", "boolean", "Optional: include addons directory (default false)")
            .prop("max_results", "integer", "Optional: maximum results (default 200)")
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String directory = args_string(ctx.args, "directory", "res://");
        bool include_addons = args_bool(ctx.args, "include_addons", false);
        int64_t max_results = args_int(ctx.args, "max_results", 200);

        Dictionary verr = fs_utils::validate_res_path(directory);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }

        Array extensions = Array::make(ext());
        Array files;
        walk_project_dir(directory, extensions, include_addons, max_results, files);

        Array result_files;
        for (int i = 0; i < files.size(); i++) {
            String file_path = files[i];
            Dictionary entry;
            entry["path"] = file_path;
            entry["name"] = fs_utils::get_file_name(file_path);
            entry["language"] = lang_id();
            result_files.append(entry);
        }

        bool truncated = result_files.size() >= max_results;

        Dictionary data;
        data["files"] = result_files;
        data["total"] = static_cast<int64_t>(result_files.size());
        data["truncated"] = truncated;
        return ToolResult::ok(data);
    }
};

using ListGdScriptsTool = ListScriptsTool<ScriptLang::GDScript>;
using ListCsharpScriptsTool = ListScriptsTool<ScriptLang::CSharp>;

} // namespace godot_mcp
