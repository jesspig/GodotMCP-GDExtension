// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class RemoveInputActionTool : public ITool {
public:
    String name() const override { return "remove_input_action"; }
    String category() const override { return "input_map"; }
    String brief() const override { return "Remove an input action"; }
    String description() const override { return "Remove an input action"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["name"] = [](){Dictionary d;d["type"]="string";d["description"]="Action name";return d;}();
        s["properties"] = p; Array r; r.push_back("name"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name");
        if (name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'name'");
        String key = "input/" + name;
        if (!ProjectSettings::get_singleton()->has_setting(key))
            return ToolResult::err("NOT_FOUND", "action '" + name + "' not found");
        ProjectSettings::get_singleton()->set_setting(key, Variant());
        ProjectSettings::get_singleton()->save();
        InputMap::get_singleton()->load_from_project_settings();
        Dictionary d; d["name"] = name; d["removed"] = true;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
