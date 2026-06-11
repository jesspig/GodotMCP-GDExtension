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
        return "Delete a file or directory";
    }
    String description() const override {
        return "Deletes the specified file or directory. Non-empty directories require recursive=true. "
               "Uses DirAccess::remove_absolute(), then notifies EditorFileSystem to refresh. "
               "Deleting res:// itself is forbidden.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "res:// path to delete (file or directory)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Optional: recursively delete directory contents (default false)";
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
                "Cannot delete the project root res://");
        }
        if (!fs_utils::path_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "Path does not exist: " + path);
        }

        // Capture before deletion â€?is_file() returns false after file is deleted
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
                "Delete failed, error code: " + String::num_int64((int64_t)err));
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
