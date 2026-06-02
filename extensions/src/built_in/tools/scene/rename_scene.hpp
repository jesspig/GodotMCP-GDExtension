// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class RenameSceneTool : public ITool {
public:
    String name() const override { return "rename_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Rename a scene file"; }
    String description() const override {
        return "Renames a .tscn scene file from source_path to dest_path. "
               "If the scene is currently open, it will be saved, closed, renamed, and reopened.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["source_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Current scene file path"; return d; }();
        props["dest_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "New scene file path"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("source_path"); req.push_back("dest_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String src = args_string(ctx.args, "source_path");
        const String dst = args_string(ctx.args, "dest_path");

        if (src == dst) return ToolResult::err("SAME_PATH", "source_path and dest_path are the same");
        if (!FileAccess::file_exists(src)) return ToolResult::err("NOT_FOUND", "source_path '" + src + "' does not exist");
        if (FileAccess::file_exists(dst)) return ToolResult::err("ALREADY_EXISTS", "dest_path '" + dst + "' already exists");

        EditorInterface *ei = EditorInterface::get_singleton();
        Array scenes = ei->get_open_scenes();
        bool is_open = false;
        for (int i = 0; i < scenes.size(); i++) {
            if ((String)scenes[i] == src) { is_open = true; break; }
        }

        if (is_open) {
            Node *r = ei->get_edited_scene_root();
            if (r && r->get_scene_file_path() != src) return ToolResult::err("NOT_ACTIVE", "Scene is open but not active tab");
            ei->save_scene();
            ei->close_scene();
        }

        Ref<DirAccess> d = DirAccess::open("res://");
        if (d.is_null()) return ToolResult::err("DIR_ACCESS", "Cannot open res://");
        if (d->rename(src, dst) != OK) return ToolResult::err("RENAME_FAILED", "Failed to rename '" + src + "' to '" + dst + "'");

        notify_file_changed(dst);

        Dictionary data;
        data["source"] = src;
        data["destination"] = dst;
        if (is_open) {
            ei->open_scene_from_path(dst);
            data["tab_reopened"] = true;
        }
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
