#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

namespace godot_mcp {

class GlobScriptsTool : public ITool {
public:
    String name() const noexcept override { return "glob_scripts"; }
    String category() const noexcept override { return "editor_tools/scripts"; }
    String brief() const noexcept override {
        return "Match script files by path pattern";
    }
    String description() const override {
        return "Match script file paths in the project using glob patterns. Supports wildcards: * (any characters), ? (single character). Supports filtering by language (gdscript/csharp/all).";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("pattern", "string", "Glob match pattern (e.g. **/enemy*)")
            .prop("language", "string", "Optional: language filter (gdscript/csharp/all, default all)")
            .prop("directory", "string", "Optional: search root directory (default res://)")
            .prop("include_addons", "boolean", "Optional: include addons directory (default false)")
            .prop("max_results", "integer", "Optional: maximum results (default 200)")
            .required(Array::make("pattern"))
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String pattern = args_string(ctx.args, "pattern");
        String language = args_string(ctx.args, "language", "all");
        String directory = args_string(ctx.args, "directory", "res://");
        bool include_addons = args_bool(ctx.args, "include_addons", false);
        int64_t max_results = args_int(ctx.args, "max_results", 200);

        if (pattern.is_empty()) {
            return ToolResult::err("MISSING_ARG",
                "pattern cannot be empty");
        }

        Dictionary verr = fs_utils::validate_res_path(directory);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }

        Array extensions;
        if (language == "gdscript") {
            extensions.append(".gd");
        } else if (language == "csharp") {
            extensions.append(".cs");
        } else {
            extensions.append(".gd");
            extensions.append(".cs");
        }

        Array all_files;
        walk_project_dir(directory, extensions, include_addons, max_results, all_files);

        Array matched;
        for (int i = 0; i < all_files.size() && matched.size() < max_results; i++) {
            String file_path = all_files[i];
            String file_name = fs_utils::get_file_name(file_path);

            if (file_name.match(pattern)) {
                Dictionary entry;
                entry["path"] = file_path;
                entry["name"] = file_name;
                entry["language"] = script_utils::detect_language(file_path);
                matched.append(entry);
            }
        }

        bool truncated = matched.size() >= max_results;

        Dictionary data;
        data["files"] = matched;
        data["total"] = static_cast<int64_t>(matched.size());
        data["truncated"] = truncated;
        data["pattern"] = pattern;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

