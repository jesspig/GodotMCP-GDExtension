
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot_mcp {

class ListPluginsTool : public ITool {
public:
    String name() const noexcept override { return "list_plugins"; }
    String category() const noexcept override { return "editor_tools/plugin"; }
    String brief() const noexcept override {
        return "List all editor plugins in the project";
    }
    String description() const override {
        return "Scan res://addons/ for plugins (plugin.cfg) and report "
               "each plugin's name, path, and enabled status.";
    }
    Dictionary build_input_schema() const override {
        Dictionary s;
        s["type"] = "object";
        s["properties"] = Dictionary();
        return s;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        (void)ctx;
        godot::Ref<godot::DirAccess> da = godot::DirAccess::open("res://addons");
        Array results;

        if (da.is_valid()) {
            da->list_dir_begin();
            while (true) {
                String n = da->get_next();
                if (n.is_empty()) break;
                if (n == "." || n == "..") continue;
                if (!da->current_is_dir()) continue;

                String plugin_cfg = "res://addons/" + n + "/plugin.cfg";
                godot::Ref<godot::ConfigFile> cfg;
                cfg.instantiate();
                if (cfg->load(plugin_cfg) != godot::OK) continue;

                String plugin_name = cfg->get_value("plugin", "name", n);
                bool enabled = false;
                {
                    // `.enabled` is a FILE marker, not a directory. DirAccess::open
                    // only opens directories, so it always returned an invalid Ref
                    // here and `enabled` was unconditionally false. Use file_exists.
                    String enabled_path = "res://addons/" + n + "/.enabled";
                    enabled = godot::FileAccess::file_exists(enabled_path);
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
        data["count"] = static_cast<int64_t>(results.size());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
