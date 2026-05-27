// commands/project_settings.cpp — project settings tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_get_project_setting(const Dictionary &a) {
    String prop = args_string(a, "property");
    if (prop.is_empty()) return make_error("missing 'property'");
    Dictionary r; r["property"] = prop; r["value"] = variant_to_json(ProjectSettings::get_singleton()->get_setting(prop)); return r;
}
Dictionary cmd_set_project_setting(const Dictionary &a) {
    String prop = args_string(a, "property");
    if (prop.is_empty()) return make_error("missing 'property'");
    if (!a.has("value")) return make_error("missing 'value'");
    ProjectSettings::get_singleton()->set_setting(prop, json_to_variant(a["value"]));
    Dictionary r; r["property"] = prop; r["value"] = a["value"];
    if (ProjectSettings::get_singleton()->save() != OK) r["warning"] = "disk write failed";
    return r;
}
Dictionary cmd_set_main_scene(const Dictionary &a) {
    String sp = args_string(a, "scene_path");
    if (sp.is_empty()) return make_error("missing 'scene_path'");
    ProjectSettings::get_singleton()->set_setting("application/run/main_scene", sp);
    Dictionary r; r["main_scene"] = sp;
    if (ProjectSettings::get_singleton()->save() != OK) r["warning"] = "disk write failed";
    return r;
}

void list_autoloads_res(const Array &props, const String &prefix, Array &out, ProjectSettings *ps) {
    for (int i = 0; i < props.size(); i++) {
        Dictionary d = props[i];
        String name = d.get("name", "");
        if (!name.begins_with(prefix)) continue;
        String aname = name.substr(prefix.length());
        if (aname.is_empty()) continue;
        String val = ps->get_setting(name);
        bool singleton = val.begins_with(String("*"));
        String path = singleton ? val.substr(1) : val;
        Dictionary e; e["name"] = aname; e["path"] = path; e["singleton"] = singleton;
        out.append(e);
    }
}
Dictionary cmd_list_autoloads(const Dictionary &) {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    Array pl = ps->get_property_list();
    Array out; list_autoloads_res(pl, "autoload/", out, ps);
    Dictionary r; r["autoloads"] = out; r["count"] = (int64_t)out.size(); return r;
}
Dictionary cmd_add_autoload(const Dictionary &a) {
    String name = args_string(a, "name"), path = args_string(a, "path");
    if (name.is_empty() || path.is_empty()) return make_error("missing 'name' or 'path'");
    bool singleton = args_bool(a, "singleton", true);
    String val = singleton ? "*" + path : path;
    ProjectSettings::get_singleton()->set_setting("autoload/" + name, val);
    Dictionary r; r["name"] = name; r["path"] = path; r["singleton"] = singleton;
    if (ProjectSettings::get_singleton()->save() != OK) r["warning"] = "disk write failed";
    return r;
}
Dictionary cmd_remove_autoload(const Dictionary &a) {
    String name = args_string(a, "name");
    if (name.is_empty()) return make_error("missing 'name'");
    ProjectSettings::get_singleton()->set_setting("autoload/" + name, Variant());
    Dictionary r; r["name"] = name; r["removed"] = true;
    if (ProjectSettings::get_singleton()->save() != OK) r["warning"] = "disk write failed";
    return r;
}

void list_scenes_rec(const String &dir, int64_t max, Array &out) {
    if (out.size() >= max) return;
    Ref<DirAccess> d = DirAccess::open(dir); if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        String full = dir == "res://" ? String("res://") + n : dir + String("/") + n;
        if (d->current_is_dir()) {
            if (n == ".godot" || n == ".import") continue;
            list_scenes_rec(full, max, out);
        } else if ((n.ends_with(".tscn") || n.ends_with(".scn")) && out.size() < max) {
            out.append(full);
        }
    }
}
Dictionary cmd_list_scenes(const Dictionary &a) {
    String root = args_string(a, "root", "res://");
    int64_t max = args_int(a, "max_results", 1000);
    Array out; list_scenes_rec(root, max, out);
    Dictionary r; r["scenes"] = out; r["count"] = (int64_t)out.size(); r["truncated"] = out.size() >= max; return r;
}
} // namespace

void register_project_settings(HandlerRegistry &reg) {
    reg.register_tool("get_project_setting", cmd_get_project_setting);
    reg.register_tool("set_project_setting", cmd_set_project_setting);
    reg.register_tool("set_main_scene", cmd_set_main_scene);
    reg.register_tool("list_autoloads", cmd_list_autoloads);
    reg.register_tool("add_autoload", cmd_add_autoload);
    reg.register_tool("remove_autoload", cmd_remove_autoload);
    reg.register_tool("list_scenes", cmd_list_scenes);
}
} // namespace godot_mcp