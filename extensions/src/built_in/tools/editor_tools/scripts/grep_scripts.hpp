#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class GrepScriptsTool : public ITool {
public:
    String name() const override { return "grep_scripts"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return "Search for text content in script files";
    }
    String description() const override {
        return "Search for a text pattern in project script files, returning matching file paths, line numbers, and line content. Supports filtering by language (gdscript/csharp/all), case sensitivity, and maximum results limit.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Search text pattern";
            props["pattern"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: language filter (gdscript/csharp/all, default all)";
            props["language"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: search root directory (default res://)";
            props["directory"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: case sensitive (default false)";
            props["case_sensitive"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Optional: maximum results (default 100)";
            props["max_results"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("pattern");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String pattern = args_string(ctx.args, "pattern");
        String language = args_string(ctx.args, "language", "all");
        String directory = args_string(ctx.args, "directory", "res://");
        bool case_sensitive = args_bool(ctx.args, "case_sensitive", false);
        int64_t max_results = args_int(ctx.args, "max_results", 100);

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

        Array script_files;
        walk_project_dir(directory, extensions, false, 10000, script_files);

        Array matches;
        String search_pattern = case_sensitive ? pattern : pattern.to_lower();

        for (int i = 0; i < script_files.size() && matches.size() < max_results; i++) {
            String file_path = script_files[i];

            Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
            if (file.is_null()) continue;

            String content = file->get_as_text();
            file->close();

            Array lines = content.split("\n", false);
            String file_lang = script_utils::detect_language(file_path);

            for (int ln = 0; ln < lines.size() && matches.size() < max_results; ln++) {
                String line = lines[ln];
                String match_line = case_sensitive ? line : line.to_lower();
                if (match_line.find(search_pattern) >= 0) {
                    Dictionary entry;
                    entry["path"] = file_path;
                    entry["line"] = (int64_t)(ln + 1);
                    entry["content"] = line;
                    entry["language"] = file_lang;
                    matches.append(entry);
                }
            }
        }

        bool truncated = matches.size() >= max_results;

        Dictionary data;
        data["matches"] = matches;
        data["total"] = (int64_t)matches.size();
        data["truncated"] = truncated;
        data["pattern"] = pattern;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

