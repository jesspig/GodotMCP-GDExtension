
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

namespace godot_mcp {

class SaveResourceAsTool : public ITool {
public:
    String name() const noexcept override { return "save_resource_as"; }
    String category() const noexcept override { return "editor_tools/filesystem"; }
    String brief() const noexcept override {
        return "Save a resource file to a target path";
    }
    String description() const override {
        return "Loads a resource from a source path and saves it to a target path "
               "using ResourceSaver. If save_path is empty, re-saves in-place. "
               "Supports .tres (text) and .res (binary) formats. Useful for "
               "duplicating resources or forcing a re-save after external edits.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("resource_path", "string", "Source resource path (res://...)")
            .prop("save_path", "string", "Target save path (empty = re-save in-place)")
            .required({"resource_path"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String resource_path = args_string(ctx.args, "resource_path");
        String save_path = args_string(ctx.args, "save_path", "");

        Dictionary verr = fs_utils::validate_res_path(resource_path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!godot::FileAccess::file_exists(resource_path)) {
            return ToolResult::err("FILE_NOT_FOUND",
                String("Resource file not found: ") + resource_path);
        }

        if (save_path.is_empty()) {
            save_path = resource_path;
        } else {
            Dictionary v2 = fs_utils::validate_res_path(save_path);
            if (!v2.is_empty()) {
                return ToolResult::err(v2["code"], v2["message"]);
            }
            if (!save_path.ends_with(".tres") && !save_path.ends_with(".res")) {
                return ToolResult::err("BAD_EXTENSION",
                    "Save path must end with .tres or .res");
            }
            if (!fs_utils::ensure_parent_dir(save_path)) {
                return ToolResult::err("MKDIR_FAILED",
                    "Failed to create parent directory");
            }
        }

        godot::Ref<godot::Resource> res = godot::ResourceLoader::get_singleton()->load(resource_path);
        if (res.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                String("Failed to load resource: ") + resource_path);
        }

        Error err = godot::ResourceSaver::get_singleton()->save(res, save_path,
            godot::ResourceSaver::FLAG_CHANGE_PATH);

        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED",
                String("Failed to save resource (error ") +
                String::num_int64(static_cast<int64_t>(err)) + String("): ") + save_path);
        }

        fs_utils::notify_file_changed(save_path);

        Dictionary data;
        data["resource_path"] = resource_path;
        data["save_path"] = save_path;
        data["in_place"] = (save_path == resource_path);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

