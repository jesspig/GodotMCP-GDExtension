// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class RemoveAutoloadTool : public ITool {
public:
    String name() const override { return "remove_autoload"; }
    String category() const override { return "project_settings"; }
    String brief() const override { return "Remove an autoload"; }
    String description() const override { return "Remove an autoload"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["name"] = [](){Dictionary d;d["type"]="string";d["description"]="Autoload name";return d;}();
        s["properties"] = p; Array r; r.push_back("name"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name");
        if (name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'name'");
        ProjectSettings::get_singleton()->set_setting("autoload/" + name, Variant());
        Dictionary d; d["name"] = name; d["removed"] = true;
        if (ProjectSettings::get_singleton()->save() != OK) d["warning"] = "disk write failed";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
