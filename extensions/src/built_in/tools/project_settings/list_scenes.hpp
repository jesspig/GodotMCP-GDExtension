// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/dir_access.hpp>

namespace godot_mcp {

class ListScenesTool : public ITool {
public:
    String name() const override { return "list_scenes"; }
    String category() const override { return "project_settings"; }
    String brief() const override { return "List all .tscn/.scn files"; }
    String description() const override { return "List all .tscn/.scn files"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["root"] = [](){Dictionary d;d["type"]="string";d["description"]="Root directory (default: res://)";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 1000)";return d;}();
        s["properties"] = p; return s;
    }
protected:
    static void list_scenes_rec(const String &dir, int64_t max, Array &out) {
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
    Dictionary execute_impl(const ToolContext &ctx) override {
        String root = args_string(ctx.args, "root", "res://");
        int64_t max = args_int(ctx.args, "max_results", 1000);
        Array out; list_scenes_rec(root, max, out);
        Dictionary d; d["scenes"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
