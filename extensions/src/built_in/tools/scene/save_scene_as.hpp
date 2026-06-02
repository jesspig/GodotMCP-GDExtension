// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class SaveSceneAsTool : public ITool {
public:
    String name() const override { return "save_scene_as"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Save the current scene to a new path"; }
    String description() const override {
        return "Saves the currently active scene to a specified file path, creating a copy.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["scene_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Target scene file path"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("scene_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String sp = args_string(ctx.args, "scene_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'scene_path'");

        EditorInterface *ei = EditorInterface::get_singleton();
        Node *root = ei->get_edited_scene_root();

        Array warnings;
        if (root) {
            warnings = collect_owner_warnings(root);
        }

        ei->save_scene_as(sp);
        if (root) save_version_marker(root);

        Dictionary data;
        data["saved"] = sp;
        if (warnings.size() > 0) data["warning"] = warnings;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
