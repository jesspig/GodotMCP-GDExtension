// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class GetProjectSettingTool : public ITool {
public:
    String name() const override { return "get_project_setting"; }
    String category() const override { return "project_settings"; }
    String brief() const override { return "Get a project setting value"; }
    String description() const override { return "Get a project setting value"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["property"] = [](){Dictionary d;d["type"]="string";d["description"]="Setting name";return d;}();
        s["properties"] = p; Array r; r.push_back("property"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'property'");
        Dictionary d; d["property"] = prop; d["value"] = variant_to_json(ProjectSettings::get_singleton()->get_setting(prop));
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
