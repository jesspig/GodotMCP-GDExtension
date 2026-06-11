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
        return "Create a directory";
    }
    String description() const override {
        return "Creates one or more directories at the specified res:// path (mkdir -p). "
               "Uses DirAccess::make_dir_recursive_absolute(). "
               "Can create nested directories in one call.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Directory path to create (res:// prefix)";
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
                "Path already exists: " + path);
        }

        Error err = DirAccess::make_dir_recursive_absolute(path);
        if (err != Error::OK) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create directory, error code: " + String::num_int64((int64_t)err));
        }

        fs_utils::notify_fs_changes();

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
