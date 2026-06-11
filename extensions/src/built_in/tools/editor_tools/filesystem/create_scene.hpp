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
        return "Create a Godot scene (.tscn) file";
    }
    String description() const override {
        return "Creates an empty .tscn scene file at the specified res:// path. "
               "Uses the PackedScene::pack() + ResourceSaver::save() workflow, "
               "consistent with Godot's SceneTreeDock official scene creation path. "
               "The scene includes a root Node by default.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target path (must end with .tscn)";
            props["path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: root node type (e.g. Node2D, Node3D, default Node)";
            props["root_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional: root node name (default Node)";
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
                "Scene path must end with .tscn"));
        }
        if (FileAccess::file_exists(path)) {
            return ToolResult::err("FILE_EXISTS",
                "File already exists: " + path);
        }
        if (!ClassDB::class_exists(root_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown node type: " + root_type);
        }
        if (!fs_utils::ensure_parent_dir(path)) {
            return ToolResult::err("MKDIR_FAILED",
                "Failed to create parent directory"));
        }

        Node *temp_root = Object::cast_to<Node>(ClassDB::instantiate(root_type));
        if (!temp_root) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + root_type);
        }
        temp_root->set_name(root_name);

        Ref<PackedScene> scene;
        scene.instantiate();
        Error err = scene->pack(temp_root);
        memdelete(temp_root);

        if (err != Error::OK) {
            return ToolResult::err("PACK_FAILED",
                "Failed to pack scene, error code: " + String::num_int64((int64_t)err));
        }

        err = ResourceSaver::get_singleton()->save(scene, path, ResourceSaver::FLAG_CHANGE_PATH);
        if (err != Error::OK) {
            return ToolResult::err("SAVE_FAILED",
                "Failed to save scene, error code: " + String::num_int64((int64_t)err));
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
