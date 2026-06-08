// @tool register
#pragma once

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
    String name() const override { return "open_file"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return String::utf8("在编辑器中打开文件");
    }
    String description() const override {
        return String::utf8("在 Godot 编辑器中打开指定文件。根据文件扩展名路由到对应的编辑器组件："
                            ".tscn → 场景编辑器；.gd → 脚本编辑器；"
                            ".gdshader → 着色器编辑器；.tres/.res → 资源编辑器；"
                            "其他文件 → 在文件系统面板中选中。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("要打开的 res:// 文件路径");
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = String::utf8("可选：脚本打开到的行号（默认 -1 = 不指定）");
            props["line"] = p;
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
        int64_t line = args_int(ctx.args, "line", -1);

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!FileAccess::file_exists(path)) {
            return ToolResult::err("NOT_FOUND",
                String::utf8("文件不存在: ") + path);
        }

        EditorInterface *ei = EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR",
                String::utf8("EditorInterface 不可用"));
        }

        String ext = fs_utils::get_file_extension(path);
        String action_desc;

        if (ext == "tscn" || ext == "scn") {
            ei->open_scene_from_path(path);
            action_desc = String::utf8("场景已打开");
        } else if (ext == "gd" || ext == "gdshader" || ext == "cs" || ext == "csharp") {
            Ref<Resource> res = ResourceLoader::get_singleton()->load(path);
            if (res.is_null()) {
                EditorFileSystem *efs = EditorInterface::get_singleton()->get_resource_filesystem();
                if (efs) {
                    efs->update_file(path);
                }
                res = ResourceLoader::get_singleton()->load(path);
            }
            if (res.is_null()) {
                return ToolResult::err("LOAD_FAILED",
                    String::utf8("无法加载文件: ") + path);
            }
            Ref<Script> script = res;
            if (script.is_valid()) {
                ei->edit_script(script, (int)line);
            } else {
                ei->edit_resource(res);
            }
            action_desc = String::utf8("文件已在脚本/资源编辑器中打开");
        } else if (ext == "tres" || ext == "res") {
            Ref<Resource> res = ResourceLoader::get_singleton()->load(path);
            if (res.is_null()) {
                EditorFileSystem *efs = EditorInterface::get_singleton()->get_resource_filesystem();
                if (efs) {
                    efs->update_file(path);
                }
                res = ResourceLoader::get_singleton()->load(path);
            }
            if (res.is_null()) {
                return ToolResult::err("LOAD_FAILED",
                    String::utf8("无法加载资源: ") + path);
            }
            ei->edit_resource(res);
            action_desc = String::utf8("资源已在资源编辑器中打开");
        } else {
            ei->select_file(path);
            action_desc = String::utf8("文件已在文件系统面板中选中");
        }

        Dictionary data;
        data["path"] = path;
        data["extension"] = ext;
        data["action"] = action_desc;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
