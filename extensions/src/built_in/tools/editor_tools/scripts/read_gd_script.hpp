#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class ReadGdScriptTool : public ITool {
public:
    String name() const override { return "read_gd_script"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return "Read GDScript (.gd) file contents";
    }
    String description() const override {
        return "Read the contents of a GDScript file at the specified path. Returns the file path, full content, line count, and language type.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target file path (must end with .gd)";
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
        if (!path.ends_with(".gd")) {
            return ToolResult::err("BAD_EXTENSION",
                "Path must end with .gd");
        }
        if (!FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "File does not exist: " + path);
        }

        Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
        if (file.is_null()) {
            return ToolResult::err("READ_FAILED",
                "Failed to open file: " + path);
        }
        String content = file->get_as_text();
        file->close();

        int64_t line_count = 1;
        for (int i = 0; i < content.length(); i++) {
            if (content[i] == '\n') line_count++;
        }

        Dictionary data;
        data["path"] = path;
        data["content"] = content;
        data["line_count"] = line_count;
        data["language"] = String("gdscript");
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

