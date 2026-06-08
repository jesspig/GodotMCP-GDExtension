// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ValidateCsharpScriptTool : public ITool {
public:
    String name() const override { return "validate_csharp_script"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return String::utf8("验证 C# Script (.cs) 文件语法");
    }
    String description() const override {
        return String::utf8("使用 dotnet build --no-restore 验证 C# 脚本文件语法。返回验证结果和退出码。需要项目中已配置 .NET SDK。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标文件路径（必须以 .cs 结尾）");
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
        if (!path.ends_with(".cs")) {
            return ToolResult::err("BAD_EXTENSION",
                String::utf8("路径必须以 .cs 结尾"));
        }
        if (!FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                String::utf8("文件不存在: ") + path);
        }

        if (!script_utils::has_dotnet()) {
            return ToolResult::err("NO_DOTNET",
                String::utf8("项目未启用 .NET，无法验证 C# 脚本"));
        }

        OS *os = OS::get_singleton();
        if (!os) {
            return ToolResult::err("NO_OS",
                String::utf8("OS 单例不可用"));
        }

        ProjectSettings *ps = ProjectSettings::get_singleton();
        String project_path = ps ? ps->globalize_path("res://") : String(".");

        Array args_arr;
        args_arr.append("build");
        args_arr.append("--no-restore");
        args_arr.append("--no-build");
        args_arr.append("--project");
        args_arr.append(project_path);

        int64_t exit_code = os->execute("dotnet", args_arr, Array(), false);

        Dictionary data;
        data["path"] = path;
        data["language"] = String("csharp");
        data["valid"] = (exit_code == 0);
        data["exit_code"] = exit_code;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
