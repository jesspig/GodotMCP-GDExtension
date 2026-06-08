// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class ReadCsharpScriptTool : public ITool {
public:
    String name() const override { return "read_csharp_script"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return String::utf8("读取 C# Script (.cs) 文件内容");
    }
    String description() const override {
        return String::utf8("读取指定路径的 C# 脚本文件内容并返回。返回内容包括文件路径、全文、行数和语言类型。");
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

        Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        if (file.is_null()) {
            return ToolResult::err("READ_FAILED",
                String::utf8("无法打开文件: ") + path);
        }
        String content = file->get_as_text();
        file->close();

        int64_t line_count = 1;
        for (int i = 0; i < content.length(); i++) {
            if (content[i] == '\n') line_count++;
        }

        Dictionary data;
        data["path"] = path;
        data["content"] = content;
        data["line_count"] = line_count;
        data["language"] = String("csharp");
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
