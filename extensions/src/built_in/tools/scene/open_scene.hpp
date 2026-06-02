// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class OpenSceneTool : public ITool {
public:
    String name() const override { return "open_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Open a scene in the editor"; }
    String description() const override {
        return "Opens a .tscn scene file in the Godot editor. Records a save version marker for dirty tracking.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["scene_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the scene file to open"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("scene_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String sp = args_string(ctx.args, "scene_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'scene_path'");
        if (!FileAccess::file_exists(sp)) return ToolResult::err("NOT_FOUND", "Scene file does not exist: " + sp);

        EditorInterface *ei = EditorInterface::get_singleton();
        ei->open_scene_from_path(sp);

        Node *root = ei->get_edited_scene_root();
        if (root) save_version_marker(root);

        Dictionary data;
        data["opened"] = sp;
        data["loaded"] = true;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
