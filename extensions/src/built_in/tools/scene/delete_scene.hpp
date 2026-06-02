// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class DeleteSceneTool : public ITool {
public:
    String name() const override { return "delete_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Delete a scene file"; }
    String description() const override {
        return "Deletes a .tscn scene file. If the scene is currently open, it will be closed first.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Scene file path to delete"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String p = args_string(ctx.args, "path");
        if (p.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'path'");

        EditorInterface *ei = EditorInterface::get_singleton();
        Array scenes = ei->get_open_scenes();
        bool is_open = false;
        for (int i = 0; i < scenes.size(); i++) {
            if ((String)scenes[i] == p) { is_open = true; break; }
        }

        if (is_open) {
            Node *r = ei->get_edited_scene_root();
            if (r && r->get_scene_file_path() != p) { ei->open_scene_from_path(p); }
            if (ei->close_scene() != OK) return ToolResult::err("CLOSE_FAILED", "Failed to close scene before deletion");
        }

        Ref<DirAccess> d = DirAccess::open("res://");
        if (d.is_null()) return ToolResult::err("DIR_ACCESS", "Cannot open res://");
        if (d->remove(p) != OK) return ToolResult::err("DELETE_FAILED", "Failed to delete '" + p + "'");

        notify_file_changed(p);

        Dictionary data;
        data["deleted"] = p;
        data["was_open"] = is_open;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
