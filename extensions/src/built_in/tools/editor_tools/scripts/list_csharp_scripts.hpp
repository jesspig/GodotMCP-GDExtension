// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

namespace godot_mcp {

class ListCsharpScriptsTool : public ITool {
public:
    String name() const override { return "list_csharp_scripts"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return String::utf8("列出项目中的 C# Script (.cs) 文件");
    }
    String description() const override {
        return String::utf8("递归遍历项目目录，列出所有 C# 脚本文件。支持按目录过滤、排除 addons、最大结果数限制。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：搜索根目录（默认 res://）");
            props["directory"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("可选：包含 addons 目录（默认 false）");
            props["include_addons"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String::utf8("可选：最大结果数（默认 200）");
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

        Array extensions = Array::make(".cs");
        Array files;
        walk_project_dir(directory, extensions, include_addons, max_results, files);

        Array result_files;
        for (int i = 0; i < files.size(); i++) {
            String file_path = files[i];
            Dictionary entry;
            entry["path"] = file_path;
            entry["name"] = fs_utils::get_file_name(file_path);
            entry["language"] = String("csharp");
            result_files.append(entry);
        }

        bool truncated = result_files.size() >= max_results;

        Dictionary data;
        data["files"] = result_files;
        data["total"] = (int64_t)result_files.size();
        data["truncated"] = truncated;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
