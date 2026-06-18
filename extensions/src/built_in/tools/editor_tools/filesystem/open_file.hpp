#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot_mcp {

class OpenFileTool : public ITool {
public:
    String name() const noexcept override { return "open_file"; }
    String category() const noexcept override { return "editor_tools/filesystem"; }
    String brief() const noexcept override {
        return "Open a file in the editor";
    }
    String description() const override {
        return "Opens the specified file in the Godot editor. Routes to the appropriate editor component based on file extension: "
               ".tscn - scene editor; .gd - script editor; "
               ".gdshader - shader editor; .tres/.res - resource editor; "
               "other files - selected in the file system panel.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("path", "string", "res:// file path to open")
            .prop("line", "integer", "Optional: line number to open the script at (default -1 = not specified)")
            .required({"path"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "path");
        int64_t line = args_int(ctx.args, "line", -1);

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!godot::FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                "File does not exist: " + path);
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR",
                "EditorInterface not available");
        }

        String ext = fs_utils::get_file_extension(path);
        String action_desc;

        if (ext == "tscn" || ext == "scn") {
            ei->open_scene_from_path(path);
            action_desc = "Scene opened";
        } else if (ext == "gd" || ext == "gdshader" || ext == "cs" || ext == "csharp") {
            godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(path);
            if (res.is_null()) {
                auto *efs = godot::EditorInterface::get_singleton()->get_resource_filesystem();
                if (efs) {
                    efs->update_file(path);
                }
                res = godot::ResourceLoader::get_singleton()->load(path);
            }
            if (res.is_null()) {
                return ToolResult::err("LOAD_FAILED",
                    "Failed to load file: " + path);
            }
            godot::Ref<godot::Script> script = res;
            if (script.is_valid()) {
                ei->edit_script(script, static_cast<int>(line));
            } else {
                ei->edit_resource(res);
            }
            action_desc = "File opened in script/resource editor";
        } else if (ext == "tres" || ext == "res") {
            godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(path);
            if (res.is_null()) {
                auto *efs = godot::EditorInterface::get_singleton()->get_resource_filesystem();
                if (efs) {
                    efs->update_file(path);
                }
                res = godot::ResourceLoader::get_singleton()->load(path);
            }
            if (res.is_null()) {
                return ToolResult::err("LOAD_FAILED",
                    "Failed to load resource: " + path);
            }
            ei->edit_resource(res);
            action_desc = "Resource opened in resource editor";
        } else {
            ei->select_file(path);
            action_desc = "File selected in file system panel";
        }

        Dictionary data;
        data["path"] = path;
        data["extension"] = ext;
        data["action"] = action_desc;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

