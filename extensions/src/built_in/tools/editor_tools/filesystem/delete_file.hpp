// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>

namespace godot_mcp {

class DeleteFileTool : public ITool {
public:
    String name() const override { return "delete_file"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return String::utf8("删除文件或目录");
    }
    String description() const override {
        return String::utf8("删除指定的文件或目录。对非空目录需要 recursive=true。"
                            "使用 DirAccess::remove_absolute()，然后通知 EditorFileSystem 刷新。"
                            "禁止删除 res:// 本身。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要删除的 res:// 路径（文件或目录）");
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("可选：递归删除目录内容（默认 false）");
            props["recursive"] = p;
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
        bool recursive = args_bool(ctx.args, "recursive", false);

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (path == "res://") {
            return ToolResult::err("ROOT_DELETE",
                String::utf8("不能删除项目根目录 res://"));
        }
        if (!fs_utils::path_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                String::utf8("路径不存在: ") + path);
        }

        // Capture before deletion — is_file() returns false after file is deleted
        bool was_dir = !fs_utils::is_file(path);

        Error err;
        if (!was_dir) {
            err = DirAccess::remove_absolute(path);
        } else if (recursive) {
            err = fs_utils::remove_recursive(path);
        } else {
            err = DirAccess::remove_absolute(path);
        }

        if (err != Error::OK) {
            return ToolResult::err("DELETE_FAILED",
                String::utf8("删除失败，错误码: ") + String::num_int64((int64_t)err));
        }

        // Re-scan after file/dir deletion
        fs_utils::notify_fs_changes();

        Dictionary data;
        data["path"] = path;
        data["was_directory"] = was_dir;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
