// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>

namespace godot_mcp {

class CopyFileTool : public ITool {
public:
    String name() const override { return "copy_file"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return String::utf8("复制文件或目录");
    }
    String description() const override {
        return String::utf8("将文件或目录从 source 复制到 destination。"
                            "文件使用 DirAccess::copy_absolute()，"
                            "目录使用递归复制（自动创建目标目录）。"
                            "包含 addons 目录时不变更。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("源路径（res:// 开头）");
            props["source"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标路径（res:// 开头）");
            props["destination"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("source", "destination");
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String source = args_string(ctx.args, "source");
        String destination = args_string(ctx.args, "destination");

        Dictionary verr_src = fs_utils::validate_res_path(source);
        if (!verr_src.is_empty()) {
            return ToolResult::err(verr_src["code"], verr_src["message"]);
        }
        Dictionary verr_dst = fs_utils::validate_res_path(destination);
        if (!verr_dst.is_empty()) {
            return ToolResult::err(verr_dst["code"], verr_dst["message"]);
        }
        if (!fs_utils::path_exists(source)) {
            return ToolResult::err("NOT_FOUND",
                String::utf8("源路径不存在: ") + source);
        }
        if (!fs_utils::ensure_parent_dir(destination)) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("无法创建目标父目录"));
        }

        Error err;
        if (fs_utils::is_file(source)) {
            err = DirAccess::copy_absolute(source, destination);
        } else {
            err = fs_utils::copy_recursive(source, destination);
        }

        if (err != Error::OK) {
            return ToolResult::err("COPY_FAILED",
                String::utf8("复制失败，错误码: ") + String::num_int64((int64_t)err));
        }

        fs_utils::notify_fs_changes();

        Dictionary data;
        data["source"] = source;
        data["destination"] = destination;
        data["is_directory"] = !fs_utils::is_file(source);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
