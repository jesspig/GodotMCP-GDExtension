// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class ListPluginsTool : public ITool {
public:
    String name() const override { return "list_plugins"; }
    String category() const override { return "plugin_management"; }
    String brief() const override { return "List all editor plugins"; }
    String category_description() const override { return String::utf8("编辑器插件的管理与查询"); }
    String description() const override { return "List all editor plugins"; }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
protected:
    Dictionary execute_impl(const ToolContext &) override {
        Ref<DirAccess> dir = DirAccess::open("res://addons/");
        if (dir.is_null()) { Dictionary d; d["plugins"] = Array(); d["count"] = 0; return ToolResult::ok(d); }
        EditorInterface *ei = EditorInterface::get_singleton();
        Array plugins;
        dir->list_dir_begin();
        while (true) {
            String name = dir->get_next();
            if (name.is_empty()) break;
            if (name == "." || name == ".." || !dir->current_is_dir()) continue;
            String cfg = "res://addons/" + name + "/plugin.cfg";
            if (!FileAccess::file_exists(cfg)) continue;
            Ref<ConfigFile> cf; cf.instantiate();
            if (cf->load(cfg) != OK) continue;
            Dictionary pd;
            pd["name"] = cf->get_value("plugin", "name", "");
            pd["directory"] = name;
            pd["description"] = cf->get_value("plugin", "description", "");
            pd["author"] = cf->get_value("plugin", "author", "");
            pd["version"] = cf->get_value("plugin", "version", "");
            pd["script"] = cf->get_value("plugin", "script", "");
            pd["enabled"] = ei->is_plugin_enabled(name);
            plugins.append(pd);
        }
        Dictionary d; d["plugins"] = plugins; d["count"] = (int64_t)plugins.size();
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
