// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class ListAutoloadsTool : public ITool {
public:
    String name() const override { return "list_autoloads"; }
    String category() const override { return "project_settings"; }
    String brief() const override { return "List all autoloads"; }
    String description() const override { return "List all autoloads"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s;
    }
protected:
    static void list_autoloads_res(const Array &props, const String &prefix, Array &out, ProjectSettings *ps) {
        for (int i = 0; i < props.size(); i++) {
            Dictionary d = props[i]; String name = (String)d.get("name", "");
            if (!name.begins_with(prefix)) continue;
            String aname = name.substr(prefix.length());
            if (aname.is_empty()) continue;
            String val = (String)ps->get_setting(name);
            bool singleton = val.begins_with("*");
            String path = singleton ? val.substr(1) : val;
            Dictionary e; e["name"] = aname; e["path"] = path; e["singleton"] = singleton;
            out.append(e);
        }
    }
    Dictionary execute_impl(const ToolContext &) override {
        ProjectSettings *ps = ProjectSettings::get_singleton();
        Array pl = ps->get_property_list();
        Array out; list_autoloads_res(pl, "autoload/", out, ps);
        Dictionary d; d["autoloads"] = out; d["count"] = (int64_t)out.size();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
