#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>

namespace godot_mcp {

class MoveFileTool : public ITool {
public:
    String name() const noexcept override { return "move_file"; }
    String category() const noexcept override { return "editor_tools/filesystem"; }
    String brief() const noexcept override {
        return "Move or rename a file/directory";
    }
    String description() const override {
        return "Moves a file or directory from source to destination. "
               "Uses DirAccess::rename_absolute(), valid for both files and directories. "
               "Supports renaming and changing directory location. "
               "Automatically creates target parent directories.";
    }
    Dictionary build_input_schema() const override {
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
                "Failed to create target parent directory");
        }

        Error err = godot::DirAccess::rename_absolute(source, destination);
        if (err != Error::OK) {
            return ToolResult::err("MOVE_FAILED",
                "Move failed, error code: " + String::num_int64(static_cast<int64_t>(err)));
        }

        fs_utils::notify_fs_changes();

        Dictionary data;
        data["source"] = source;
        data["destination"] = destination;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

