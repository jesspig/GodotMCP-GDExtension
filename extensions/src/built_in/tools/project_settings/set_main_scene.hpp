// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class SetMainSceneTool : public ITool {
public:
    String name() const override { return "set_main_scene"; }
    String category() const override { return "settings/core"; }
    String brief() const override { return "Set the main scene"; }
    String description() const override { return "Set the main scene"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["scene_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Path to main scene";return d;}();
        s["properties"] = p; Array r; r.push_back("scene_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String sp = args_string(ctx.args, "scene_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'scene_path'");
        ProjectSettings::get_singleton()->set_setting("application/run/main_scene", sp);
        Dictionary d; d["main_scene"] = sp;
        if (ProjectSettings::get_singleton()->save() != OK) d["warning"] = "disk write failed";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
