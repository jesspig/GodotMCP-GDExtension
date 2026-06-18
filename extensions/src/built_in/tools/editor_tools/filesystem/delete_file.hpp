#pragma once
#pragma warning(disable: 4828)  // non-UTF-8 bytes in file (known, harmless)

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>

namespace godot_mcp {

class DeleteFileTool : public ITool {
public:
    String name() const noexcept override { return "delete_file"; }
    String category() const noexcept override { return "editor_tools/filesystem"; }
    String brief() const noexcept override {
        return "Delete a file or directory";
    }
    String description() const override {
        return "Deletes the specified file or directory. Non-empty directories require recursive=true. "
               "Uses DirAccess::remove_absolute(), then notifies EditorFileSystem to refresh. "
               "Deleting res:// itself is forbidden.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("path", "string", "res:// path to delete (file or directory)")
            .prop("recursive", "boolean", "Optional: recursively delete directory contents (default false)")
            .required({"path"})
            .build();
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

        // Capture before deletion ??is_file() returns false after file is deleted
        bool was_dir = !fs_utils::is_file(path);

        Error err;
        if (!was_dir) {
            err = godot::DirAccess::remove_absolute(path);
        } else if (recursive) {
            err = fs_utils::remove_recursive(path);
        } else {
            err = godot::DirAccess::remove_absolute(path);
        }

        if (err != Error::OK) {
            return ToolResult::err("DELETE_FAILED",
                "Delete failed, error code: " + String::num_int64(static_cast<int64_t>(err)));
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
