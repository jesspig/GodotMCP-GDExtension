// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class AddAutoloadTool : public ITool {
public:
    String name() const override { return "add_autoload"; }
    String category() const override { return "project_settings"; }
    String brief() const override { return "Add an autoload"; }
    String category_description() const override { return String::utf8("项目配置的读取与修改"); }
    String description() const override { return "Add an autoload"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["name"] = [](){Dictionary d;d["type"]="string";d["description"]="Autoload name";return d;}();
        p["path"] = [](){Dictionary d;d["type"]="string";d["description"]="Scene/script path";return d;}();
        p["singleton"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Register as singleton";return d;}();
        s["properties"] = p; Array r; r.push_back("name"); r.push_back("path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String name = args_string(ctx.args, "name"), path = args_string(ctx.args, "path");
        if (name.is_empty() || path.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'name' or 'path'");
        bool singleton = args_bool(ctx.args, "singleton", true);
        ProjectSettings::get_singleton()->set_setting("autoload/" + name, singleton ? "*" + path : path);
        Dictionary d; d["name"] = name; d["path"] = path; d["singleton"] = singleton;
        if (ProjectSettings::get_singleton()->save() != OK) d["warning"] = "disk write failed";
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
