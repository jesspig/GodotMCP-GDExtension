// commands/plugin_management.cpp — list_plugins, set_plugin_enabled

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_list_plugins(const Dictionary &) {
    Ref<DirAccess> dir = DirAccess::open("res://addons/");
    if (dir.is_null()) { Dictionary r; r["plugins"] = Array(); r["count"] = 0; return r; }
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
    Dictionary r; r["plugins"] = plugins; r["count"] = (int64_t)plugins.size(); return r;
}
Dictionary cmd_set_plugin_enabled(const Dictionary &a) {
    String plugin = args_string(a, "plugin");
    if (plugin.is_empty()) return make_error("missing 'plugin'");
    bool enabled = args_bool(a, "enabled", true);
    EditorInterface *ei = EditorInterface::get_singleton();
    bool was = ei->is_plugin_enabled(plugin);
    ei->set_plugin_enabled(plugin, enabled);
    Dictionary r; r["plugin"] = plugin; r["enabled"] = enabled; r["previous"] = was; return r;
}
} // namespace

void register_plugin_management(HandlerRegistry &reg) {
    reg.register_tool("list_plugins", cmd_list_plugins);
    reg.register_tool("set_plugin_enabled", cmd_set_plugin_enabled);
}
} // namespace godot_mcp