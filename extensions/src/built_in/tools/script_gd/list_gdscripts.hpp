// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

inline void list_gd_rec(const String &dir, bool inc_addons, int64_t max, Array &out) {
    if (out.size() >= max) return;
    Ref<DirAccess> d = DirAccess::open(dir); if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        String n = d->get_next(); if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        String full = dir == "res://" ? String("res://") + n : dir + String("/") + n;
        if (d->current_is_dir()) { if (!inc_addons && (n == "addons" || n == ".godot" || n == ".import")) continue; list_gd_rec(full, inc_addons, max, out); }
        else if (n.ends_with(".gd") && out.size() < max) { Dictionary e; e["path"] = full; e["size"] = (int64_t)FileAccess::get_file_as_bytes(full).size(); out.append(e); }
    }
}

class ListGDScriptsTool : public ITool {
public:
    String name() const override { return "list_gdscripts"; }
    String category() const override { return "script/gdscript"; }
    String brief() const override { return "List all .gd files in the project"; }
    String description() const override { return "Recursively finds all GDScript files under the given root."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["root"] = [](){Dictionary d;d["type"]="string";d["description"]="Root directory (default: res://)";return d;}();
        p["include_addons"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Include addons/";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 1000)";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String root = args_string(ctx.args, "root", "res://");
        bool inc = args_bool(ctx.args, "include_addons", false);
        int64_t max = args_int(ctx.args, "max_results", 1000);
        Array out; list_gd_rec(root, inc, max, out);
        Dictionary d; d["scripts"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
