#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateResourceTool : public ITool {
public:
    String name() const override { return "create_resource"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return "Create a Godot resource file (.tres / .res)";
    }
    String description() const override {
        return "Creates a Godot resource file at the specified res:// path. "
               "Uses the ResourceSaver::save() workflow, consistent with Godot's official ResourceSaver path. "
               "Supports .tres (text format) and .res (binary format). "
               "The resource_type parameter can specify a Resource subclass (e.g. StyleBoxFlat, Curve, Gradient).";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target path (ending with .tres or .res)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: Resource subclass name (e.g. StyleBoxFlat, Curve, Gradient, default Resource)";
            props["resource_type"] = p;
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
        String resource_type = args_string(ctx.args, "resource_type", "Resource");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(".tres") && !path.ends_with(".res")) {
            return ToolResult::err("BAD_EXTENSION",
                "Resource path must end with .tres or .res");
        }
        if (godot::FileAccess::file_exists(path)) {
            return ToolResult::err("FILE_EXISTS",
                "File already exists: " + path);
        }
        if (!ClassDB::class_exists(resource_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown resource type: " + resource_type);
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory");
        }

        // Method 1: create via ClassDB::instantiate()
        // ClassDB::instantiate() creates engine objects without proper godot-cpp
        // Resource binding, making ResourceSaver::save() fail with ERR_QUERY_FAILED (31).
        // Use memnew(Resource) which creates a native godot-cpp Resource with full binding.
        godot::Ref<godot::Resource> res = memnew(godot::Resource);

        // Write the .tres file manually when a specific resource type is requested,
        // since we can't dynamically call memnew<Gradient>() etc. from a string.
        if (resource_type != "Resource") {
            String content = "[gd_resource type=\"" + resource_type + "\" format=3]\n[resource]\n";
            godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
            if (file.is_null()) {
                return ToolResult::err("WRITE_FAILED",
                    "Failed to write file: " + path);
            }
            file->store_string(content);
            file->close();
            fs_utils::notify_file_changed(path);
            Dictionary data;
            data["path"] = path;
            data["name"] = fs_utils::get_file_name(path);
            data["resource_type"] = resource_type;
            data["method"] = "manual_tres";
            return ToolResult::ok(data);
        }

        Error err = godot::ResourceSaver::get_singleton()->save(res, path, godot::ResourceSaver::FLAG_CHANGE_PATH);
        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED", String("Failed to save resource: error ") + godot::itos(err));
        }

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["resource_type"] = resource_type;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

