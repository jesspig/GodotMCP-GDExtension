// @tool register
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
        return String::utf8("创建文件（自动根据扩展名分派）");
    }
    String description() const override {
        return String::utf8("根据路径扩展名自动选择创建策略的复合工具："
                            ".tscn → PackedScene::pack() + ResourceSaver::save() "
                            ".tres/.res → ResourceSaver::save() "
                            ".gd/.gdshader/.cs/.csharp → FileAccess 写入文本 "
                            "其他 → 创建空文件。"
                            "是专用工具（create_gd_script / create_scene 等）的兜底入口。"
                            "专用工具提供更精细的控制参数，优先使用。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标路径（res:// 开头，扩展名决定创建策略）");
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：文件内容（用于 .gd/.gdshader/.cs 等文本文件）");
            props["content"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：创建场景时的根节点类型（默认 Node）");
            props["root_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：创建资源时的 Resource 子类名（默认 Resource）");
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
        if (FileAccess::file_exists(path)) {
            return ToolResult::err("FILE_EXISTS",
                String::utf8("文件已存在: ") + path);
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("无法创建父目录"));
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
                String::utf8("未知的节点类型: ") + root_type);
        }

        Node *temp_root = Object::cast_to<Node>(ClassDB::instantiate(root_type));
        if (!temp_root) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + root_type + String::utf8(" 的节点"));
        }
        temp_root->set_name(root_type);

        Ref<PackedScene> scene;
        scene.instantiate();
        Error err = scene->pack(temp_root);
        memdelete(temp_root);

        if (err != Error::OK) {
            return ToolResult::err("PACK_FAILED",
                String::utf8("打包场景失败，错误码: ") + String::num_int64((int64_t)err));
        }

        err = ResourceSaver::get_singleton()->save(scene, path, ResourceSaver::FLAG_CHANGE_PATH);
        if (err != Error::OK) {
            return ToolResult::err("SAVE_FAILED",
                String::utf8("保存场景失败，错误码: ") + String::num_int64((int64_t)err));
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
                String::utf8("未知的资源类型: ") + resource_type);
        }

        Resource *res_obj = Object::cast_to<Resource>(ClassDB::instantiate(resource_type));
        if (!res_obj) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + resource_type + String::utf8(" 的资源"));
        }
        Ref<Resource> res;
        res.reference_ptr(res_obj);

        Error err = ResourceSaver::get_singleton()->save(res, path, ResourceSaver::FLAG_CHANGE_PATH);
        if (err != Error::OK) {
            return ToolResult::err("SAVE_FAILED",
                String::utf8("保存资源失败，错误码: ") + String::num_int64((int64_t)err));
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
            if (ext == "gd") {
                actual_content = String("extends Node\n\n")
                    + String::utf8("# 自动创建的 GDScript\n\n")
                    + String("func _ready():\n\tpass\n");
            } else if (ext == "gdshader") {
                actual_content = String("shader_type canvas_item;\n\n")
                    + String("void fragment() {\n")
                    + String("\tCOLOR = vec4(1.0);\n")
                    + String("}\n");
            } else if (ext == "cs" || ext == "csharp") {
                String class_name = fs_utils::get_file_base_name(path);
                String sanitized;
                for (int i = 0; i < class_name.length(); i++) {
                    char32_t c = class_name.unicode_at(i);
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_') {
                        sanitized += String::chr(c);
                    }
                }
                if (sanitized.is_empty()) sanitized = "ScriptClass";
                if (sanitized.length() > 0 && sanitized.unicode_at(0) >= '0' && sanitized.unicode_at(0) <= '9') sanitized = String("_") + sanitized;
                actual_content = String("using Godot;\n\n")
                    + String("public partial class ") + sanitized + String(" : Node\n")
                    + String("{\n")
                    + String("}\n");
            } else {
                actual_content = String();
            }
        }

        Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
        if (file.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法打开文件进行写入"));
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
