// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "filesystem_utils.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateSceneTool : public ITool {
public:
    String name() const override { return "create_scene"; }
    String category() const override { return "editor_tools/filesystem"; }
    String brief() const override {
        return String::utf8("创建 Godot 场景 (.tscn) 文件");
    }
    String description() const override {
        return String::utf8("在指定的 res:// 路径创建一个空的 .tscn 场景文件。"
                            "使用 PackedScene::pack() + ResourceSaver::save() 流程，"
                            "与 Godot 的 SceneTreeDock 创建场景的官方路径一致。"
                            "场景默认包含一个根 Node 节点。");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("目标路径（必须以 .tscn 结尾）");
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：根节点类型（如 Node2D、Node3D，默认 Node）");
            props["root_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("可选：根节点名称（默认 Node）");
            props["root_name"] = p;
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
        String root_type = args_string(ctx.args, "root_type", "Node");
        String root_name = args_string(ctx.args, "root_name", "Node");

        Dictionary verr = fs_utils::validate_res_path(path);
        if (!verr.is_empty()) {
            return ToolResult::err(verr["code"], verr["message"]);
        }
        if (!path.ends_with(".tscn")) {
            return ToolResult::err("BAD_EXTENSION",
                String::utf8("场景路径必须以 .tscn 结尾"));
        }
        if (FileAccess::file_exists(path)) {
            return ToolResult::err("FILE_EXISTS",
                String::utf8("文件已存在: ") + path);
        }
        if (!ClassDB::class_exists(root_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                String::utf8("未知的节点类型: ") + root_type);
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                String::utf8("无法创建父目录"));
        }

        Node *temp_root = Object::cast_to<Node>(ClassDB::instantiate(root_type));
        if (!temp_root) {
            return ToolResult::err("CREATE_FAILED",
                String::utf8("无法创建类型为 ") + root_type + String::utf8(" 的节点"));
        }
        temp_root->set_name(root_name);

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
        data["root_type"] = root_type;
        data["root_name"] = root_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
