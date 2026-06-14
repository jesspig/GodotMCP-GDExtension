#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scripts/script_utils.hpp"
#include "built_in/tools/editor_tools/filesystem/filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class WriteCsharpScriptTool : public ITool {
public:
    String name() const override { return "write_csharp_script"; }
    String category() const override { return "editor_tools/scripts"; }
    String brief() const override {
        return "Write/Create C# Script (.cs) file";
    }
    String description() const override {
        return "Create or overwrite a C# script file. Writes directly when content is provided; creates a minimal valid script when content is empty.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target path (must end with .cs)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: script content (empty = create minimal script)";
            props["content"] = p;
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

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(".cs")) {
            return ToolResult::err("BAD_EXTENSION",
                "Path must end with .cs");
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory");
        }

        if (content.is_empty()) {
            String class_name = script_utils::sanitize_class_name(script_utils::get_file_base_name(path));
            content = String("using Godot;\n\nnamespace Game;\n\npublic partial class ") + class_name + String(" : Node\n{\n}\n");
        }

        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("WRITE_FAILED",
                "Failed to open file for writing");
        }
        file->store_string(content);
        file->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["language"] = String("csharp");
        data["size"] = static_cast<int64_t>(content.length());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

