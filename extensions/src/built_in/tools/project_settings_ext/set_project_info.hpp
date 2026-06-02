// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/project_settings_ext/project_settings_ext_helpers.hpp"

namespace godot_mcp {

class SetProjectInfoTool : public ITool {
public:
    String name() const override { return "set_project_info"; }
    String category() const override { return "settings/extended"; }
    String brief() const override { return "Set project info fields"; }
    String description() const override { return "Set project info fields"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["name"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["description"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["version"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["icon"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["main_scene"] = [](){Dictionary d;d["type"]="string";return d;}();
        p["use_custom_user_dir"] = [](){Dictionary d;d["type"]="boolean";return d;}();
        p["custom_user_dir_name"] = [](){Dictionary d;d["type"]="string";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override { return ToolResult::ok(apply_and_save(ctx.args, project_info_keys())); }
};

} // namespace godot_mcp
