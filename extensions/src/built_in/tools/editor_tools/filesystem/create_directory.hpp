// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>

namespace godot_mcp {

class CreateDirectoryTool : public ITool {
public:
    String name() const override { return "create_directory"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return String::utf8("创建目录");
    }
    String description() const override {
        return String::utf8("在指定的 res:// 路径创建一个或多个目录（mkdir -p）。"
                            "使用 DirAccess::make_dir_recursive_absolute() 实现。"
                            "可以一次创建嵌套目录。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要创建的目录路径（res:// 开头）");
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
        if (fs_utils::path_exists(path)) {
            return ToolResult::err("ALREADY_EXISTS",
                String::utf8("路径已存在: ") + path);
        }

        Error err = DirAccess::make_dir_recursive_absolute(path);
        if (err != Error::OK) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("创建目录失败，错误码: ") + String::num_int64((int64_t)err));
        }

        fs_utils::notify_fs_changes();

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
