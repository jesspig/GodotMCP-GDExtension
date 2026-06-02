// @tool register
#pragma once

#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ReloadSceneTool : public ITool {
public:
    String name() const override { return "reload_scene"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Reload a scene from disk"; }
    String description() const override {
        return "Reloads a scene file from disk, discarding any in-memory changes.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["scene_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Scene file path to reload"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("scene_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String sp = args_string(ctx.args, "scene_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'scene_path'");

        EditorInterface::get_singleton()->reload_scene_from_path(sp);

        Dictionary data;
        data["reloaded"] = sp;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
