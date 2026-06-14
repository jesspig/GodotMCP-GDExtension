
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

namespace godot_mcp {

class ListGdScriptsTool : public ITool {
public:
    String name() const override { return "list_gd_scripts"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return "List GDScript (.gd) files in the project";
    }
    String description() const override {
        return "Recursively traverse the project directory and list all GDScript files. Supports filtering by directory, excluding addons, and maximum results limit.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: search root directory (default res://)";
            props["directory"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: include addons directory (default false)";
            props["include_addons"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Optional: maximum results (default 200)";
            props["max_results"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
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

        Array extensions = Array::make(".gd");
        Array files;
        walk_project_dir(directory, extensions, include_addons, max_results, files);

        Array result_files;
        for (int i = 0; i < files.size(); i++) {
            String file_path = files[i];
            Dictionary entry;
            entry["path"] = file_path;
            entry["name"] = fs_utils::get_file_name(file_path);
            entry["language"] = String("gdscript");
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

} // namespace godot_mcp
