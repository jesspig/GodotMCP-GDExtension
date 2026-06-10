// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/config_file.hpp>

namespace godot_mcp {

class ListPluginsTool : public ITool {
public:
    String name() const override { return "list_plugins"; }
    String category() const override { return "editor_tools/plugin"; }
    String brief() const override {
        return "List all editor plugins in the project";
    }
    String description() const override {
        return "Scan res://addons/ for plugins (plugin.cfg) and report "
               "each plugin's name, path, and enabled status.";
    }
    Dictionary input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Ref<DirAccess> da = DirAccess::open("res://addons");
        Array results;

        if (da.is_valid()) {
            da->list_dir_begin();
            while (true) {
                String n = da->get_next();
                if (n.is_empty()) break;
                if (n == "." || n == "..") continue;
                if (!da->current_is_dir()) continue;

                String plugin_cfg = "res://addons/" + n + "/plugin.cfg";
                Ref<ConfigFile> cfg;
                cfg.instantiate();
                if (cfg->load(plugin_cfg) != OK) continue;

                String plugin_name = cfg->get_value("plugin", "name", n);
                bool enabled = false;
                {
                    String enabled_path = "res://addons/" + n + "/.enabled";
                    Ref<DirAccess> check = DirAccess::open(enabled_path);
                    enabled = check.is_valid();
                }

                Dictionary entry;
                entry["name"] = plugin_name;
                entry["path"] = "res://addons/" + n;
                entry["dir"] = n;
                entry["enabled"] = enabled;
                entry["author"] = cfg->get_value("plugin", "author", "");
                entry["version"] = cfg->get_value("plugin", "version", "");
                entry["description"] = cfg->get_value("plugin", "description", "");
                results.append(entry);
            }
            da->list_dir_end();
        }

        Dictionary data;
        data["plugins"] = results;
        data["count"] = (int64_t)results.size();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
