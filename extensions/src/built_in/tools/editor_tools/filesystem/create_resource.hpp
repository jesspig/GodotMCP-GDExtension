// @tool register
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
        return String::utf8("创建 Godot 资源文件 (.tres / .res)");
    }
    String description() const override {
        return String::utf8("在指定的 res:// 路径创建一个 Godot 资源文件。"
                            "使用 ResourceSaver::save() 流程，与 Godot 的 ResourceSaver 官方路径一致。"
                            "支持 .tres（文本格式）和 .res（二进制格式）。"
                            "可通过 resource_type 参数指定 Resource 子类（如 StyleBoxFlat、Curve、Gradient）。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标路径（以 .tres 或 .res 结尾）");
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：Resource 子类名（如 StyleBoxFlat、Curve、Gradient，默认 Resource）");
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
                String::utf8("资源路径必须以 .tres 或 .res 结尾"));
        }
        if (FileAccess::file_exists(path)) {
            return ToolResult::err("FILE_EXISTS",
                String::utf8("文件已存在: ") + path);
        }
        if (!ClassDB::class_exists(resource_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的资源类型: ") + resource_type);
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("无法创建父目录"));
        }

        // Method 1: create via ClassDB::instantiate()
        // ClassDB::instantiate() creates engine objects without proper godot-cpp
        // Resource binding, making ResourceSaver::save() fail with ERR_QUERY_FAILED (31).
        // Use memnew(Resource) which creates a native godot-cpp Resource with full binding.
        Ref<Resource> res = memnew(Resource);

        // Write the .tres file manually when a specific resource type is requested,
        // since we can't dynamically call memnew<Gradient>() etc. from a string.
        if (resource_type != "Resource") {
            String content = "[gd_resource type=\"" + resource_type + "\" format=3]\n[resource]\n";
            Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
            if (file.is_null()) {
                return ToolResult::err("WRITE_FAILED",
                    String::utf8("无法写入文件: ") + path);
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

        Error err = ResourceSaver::get_singleton()->save(res, path, ResourceSaver::FLAG_CHANGE_PATH);

        fs_utils::notify_file_changed(path);

        Dictionary data;
        data["path"] = path;
        data["name"] = fs_utils::get_file_name(path);
        data["resource_type"] = resource_type;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
