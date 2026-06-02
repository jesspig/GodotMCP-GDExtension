// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class SetProjectSettingTool : public ITool {
public:
    String name() const override { return "set_project_setting"; }
    String category() const override { return "settings/core"; }
    String brief() const override { return "Set a project setting value"; }
    String description() const override { return "Set a project setting value"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["property"] = [](){Dictionary d;d["type"]="string";d["description"]="Setting name";return d;}();
        p["value"] = [](){Dictionary d;d["type"]="object";d["description"]="New value";return d;}();
        s["properties"] = p; Array r; r.push_back("property"); r.push_back("value"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'property'");
        if (!ctx.args.has("value")) return ToolResult::err("MISSING_PARAM", "missing 'value'");
        ProjectSettings::get_singleton()->set_setting(prop, json_to_variant(ctx.args["value"]));
        Dictionary d; d["property"] = prop; d["value"] = ctx.args["value"];
        if (ProjectSettings::get_singleton()->save() != OK) d["warning"] = "disk write failed";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
