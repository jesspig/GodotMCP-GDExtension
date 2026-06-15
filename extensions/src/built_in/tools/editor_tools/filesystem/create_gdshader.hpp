#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class CreateGdshaderTool : public ITool {
public:
    String name() const noexcept override { return "create_gdshader"; }
    String category() const noexcept override { return "editor_tools/filesystem"; }
    String brief() const noexcept override {
        return "Create a Godot shader (.gdshader) file";
    }
    String description() const override {
        return "Creates a Godot shader file at the specified res:// path. "
               "Uses FileAccess to write text content, then notifies EditorFileSystem to refresh. "
               "When content is not provided, uses the default canvas_item shader template.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target path (must end with .gdshader)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: shader code (empty uses default canvas_item template)";
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
        if (!path.ends_with(".gdshader")) {
            return ToolResult::err("BAD_EXTENSION",
                "Path must end with .gdshader");
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory");
        }
        if (content.is_empty()) {
            content = String("shader_type canvas_item;\n")
                + String("render_mode blend_mix;\n\n")
                + String("void fragment() {\n")
                + String("\tCOLOR = vec4(1.0);\n")
                + String("}\n");
        }

        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to open file for writing");
        }
        file->store_string(content);
        file->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

