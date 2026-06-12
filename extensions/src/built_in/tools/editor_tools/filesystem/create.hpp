#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateTool : public ITool {
public:
    String name() const override { return "create"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return "Create a file (auto-dispatch by extension)";
    }
    String description() const override {
        return "A composite tool that selects the creation strategy based on the path extension: "
               ".tscn �?PackedScene::pack() + ResourceSaver::save() "
               ".tres/.res �?ResourceSaver::save() "
               ".gdshader �?FileAccess writes text "
               "Other �?creates an empty file. "
               "For .gd/.cs scripts, use the dedicated write_gd_script / write_csharp_script tools.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target path (res:// prefix, extension determines strategy)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: file content (for .gd/.gdshader/.cs and other text files)";
            props["content"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: root node type when creating a scene (default Node)";
            props["root_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: Resource subclass name when creating a resource (default Resource)";
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
        String content = args_string(ctx.args, "content");
        String root_type = args_string(ctx.args, "root_type", "Node");
        String resource_type = args_string(ctx.args, "resource_type", "Resource");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (godot::FileAccess::file_exists(path)) {
            return ToolResult::err("FILE_EXISTS",
                "File already exists: " + path);
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory");
        }

        String ext = fs_utils::get_file_extension(path);

        if (ext == "tscn" || ext == "scn") {
            return create_scene(path, root_type);
        } else if (ext == "tres" || ext == "res") {
            return create_resource(path, resource_type);
        } else {
            return create_text_file(path, content, ext);
        }
    }

private:
    Dictionary create_scene(const String &path, const String &root_type) {
        if (!ClassDB::class_exists(root_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown node type: " + root_type);
        }

        Node *temp_root = Object::cast_to<Node>(ClassDB::instantiate(root_type));
        if (!temp_root) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + root_type);
        }
        temp_root->set_name(root_type);

        godot::Ref<godot::PackedScene> scene;
        scene.instantiate();
        Error err = scene->pack(temp_root);
        memdelete(temp_root);

        if (err != Error::OK) {
            return ToolResult::err("PACK_FAILED",
                "Failed to pack scene, error code: " + String::num_int64((int64_t)err));
        }

        err = godot::ResourceSaver::get_singleton()->save(scene, path, godot::ResourceSaver::FLAG_CHANGE_PATH);
        if (err != Error::OK) {
            return ToolResult::err("SAVE_FAILED",
                "Failed to save scene, error code: " + String::num_int64((int64_t)err));
        }

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["type"] = "scene";
        return ToolResult::ok(data);
    }

    Dictionary create_resource(const String &path, const String &resource_type) {
        if (!ClassDB::class_exists(resource_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown resource type: " + resource_type);
        }

        godot::Resource *res_obj = Object::cast_to<godot::Resource>(ClassDB::instantiate(resource_type));
        if (!res_obj) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create resource of type: " + resource_type);
        }
        godot::Ref<godot::Resource> res;
        res.reference_ptr(res_obj);

        Error err = godot::ResourceSaver::get_singleton()->save(res, path, godot::ResourceSaver::FLAG_CHANGE_PATH);
        if (err != Error::OK) {
            return ToolResult::err("SAVE_FAILED",
                "Failed to save resource, error code: " + String::num_int64((int64_t)err));
        }

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["type"] = "resource";
        return ToolResult::ok(data);
    }

    Dictionary create_text_file(const String &path, const String &content,
                                const String &ext) {
        String actual_content = content;
        if (actual_content.is_empty()) {
            if (ext == "gdshader") {
                actual_content = String("shader_type canvas_item;\n\n")
                    + String("void fragment() {\n")
                    + String("\tCOLOR = vec4(1.0);\n")
                    + String("}\n");
            } else {
                actual_content = String();
            }
        }

        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to open file for writing");
        }
        file->store_string(actual_content);
        file->close();

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["type"] = "text";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

