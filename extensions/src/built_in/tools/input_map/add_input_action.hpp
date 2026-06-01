// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class AddInputActionTool : public ITool {
public:
    String name() const override { return "add_input_action"; }
    String category() const override { return "input_map"; }
    String brief() const override { return "Add a new input action"; }
    String description() const override { return "Add a new input action"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["name"] = [](){Dictionary d;d["type"]="string";d["description"]="Action name";return d;}();
        p["deadzone"] = [](){Dictionary d;d["type"]="number";d["description"]="Deadzone (default: 0.5)";return d;}();
        s["properties"] = p; Array r; r.push_back("name"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name");
        if (name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'name'");
        float dz = args_float(ctx.args, "deadzone", 0.5f);
        String key = "input/" + name;
        if (ProjectSettings::get_singleton()->has_setting(key))
            return ToolResult::err("ALREADY_EXISTS", "action '" + name + "' already exists");
        Dictionary ad; ad["deadzone"] = dz; ad["events"] = Array();
        ProjectSettings::get_singleton()->set_setting(key, ad);
        Dictionary d; d["name"] = name; d["deadzone"] = dz;
        if (ProjectSettings::get_singleton()->save() != OK) d["warning"] = "disk write failed";
        InputMap::get_singleton()->load_from_project_settings();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
