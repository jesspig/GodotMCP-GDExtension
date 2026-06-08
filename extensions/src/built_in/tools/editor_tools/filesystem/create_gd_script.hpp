// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class CreateGdScriptTool : public ITool {
public:
    String name() const override { return "create_gd_script"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return String::utf8("创建 GDScript (.gd) 文件");
    }
    String description() const override {
        return String::utf8("在指定的 res:// 路径创建一个 GDScript 文件。"
                            "使用 FileAccess 写入文本内容，然后通知 EditorFileSystem 刷新。"
                            "不提供 content 时使用默认模板。");
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
            p["description"] = String::utf8("可选：脚本内容（留空使用默认模板）");
            props["content"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：继承的父类（默认 Node）");
            props["extends"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：类名（注册为全局类）");
            props["class_name"] = p;
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
        String extends = args_string(ctx.args, "extends");
        String class_name = args_string(ctx.args, "class_name");

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
            if (extends.is_empty()) {
                extends = String("Node");
            }
            content = String("extends ") + extends + String("\n");
            if (!class_name.is_empty()) {
                content += String("class_name ") + class_name + String("\n");
            }
            content += String("\n")
                + String::utf8("# 这是自动生成的 GDScript 文件\n")
                + String("\n\nfunc _ready():\n\tpass\n");
        }

        Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法打开文件进行写入"));
        }
        file->store_string(content);
        file->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
