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
        return "Copy a file or directory";
    }
    String description() const override {
        return "Copies a file or directory from source to destination. "
               "Files use DirAccess::copy_absolute(), "
               "directories use recursive copy (automatically creates target directories). "
               "The addons directory is not modified.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Source path (res:// prefix)";
            props["source"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Destination path (res:// prefix)";
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
                "Source path does not exist: " + source);
        }
        if (!fs_utils::ensure_parent_dir(destination)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create target parent directory"));
        }

        Error err;
        if (fs_utils::is_file(source)) {
            err = DirAccess::copy_absolute(source, destination);
        } else {
            err = fs_utils::copy_recursive(source, destination);
        }

        if (err != Error::OK) {
            return ToolResult::err("COPY_FAILED",
                "Copy failed, error code: " + String::num_int64((int64_t)err));
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
