// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class WriteGdScriptTool : public ITool {
public:
    String name() const override { return "write_gd_script"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return String::utf8("写入/创建 GDScript (.gd) 文件");
    }
    String description() const override {
        return String::utf8("创建或覆盖一个 GDScript 文件。提供 content 时直接写入；不提供 content 时创建最小合法脚本（extends Node）。AI 客户端应自行提供完整脚本内容。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标路径（必须以 .gd 结尾）");
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：脚本内容（留空则创建最小脚本）");
            props["content"] = p;
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
        String content = args_string(ctx.args, "content");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(".gd")) {
            return ToolResult::err("BAD_EXTENSION",
                String::utf8("路径必须以 .gd 结尾"));
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("无法创建父目录"));
        }

        if (content.is_empty()) {
            content = String("extends Node\n");
        }

        Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("WRITE_FAILED",
                String::utf8("无法打开文件进行写入"));
        }
        file->store_string(content);
        file->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["language"] = String("gdscript");
        data["size"] = (int64_t)content.length();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
