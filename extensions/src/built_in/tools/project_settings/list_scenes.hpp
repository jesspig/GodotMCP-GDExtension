// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class ListScenesTool : public ITool {
public:
    String name() const override { return "list_scenes"; }
    String category() const override { return "settings/core"; }
    String brief() const override { return "List all .tscn/.scn files"; }
    String description() const override { return "List all .tscn/.scn files"; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["root"] = [](){Dictionary d;d["type"]="string";d["description"]="Root directory (default: res://)";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 1000)";return d;}();
        s["properties"] = p; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String root = args_string(ctx.args, "root", "res://");
        int64_t max = args_int(ctx.args, "max_results", 1000);
        Array exts; exts.append(".tscn"); exts.append(".scn");
        Array out; walk_project_dir(root, exts, true, max, out);
        Dictionary d; d["scenes"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace godot_mcp
