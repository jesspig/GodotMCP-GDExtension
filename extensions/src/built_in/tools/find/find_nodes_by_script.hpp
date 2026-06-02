// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class FindNodesByScriptTool : public ITool {
public:
    String name() const override { return "find_nodes_by_script"; }
    String category() const override { return "find"; }
    String brief() const override { return "Search nodes by attached script path"; }
    bool needs_scene() const override { return true; }
    String description() const override { return "Finds all nodes whose attached script path contains the given string."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["script_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Script path search pattern";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 100)";return d;}();
        s["properties"] = p; Array r; r.push_back("script_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String sp = args_string(ctx.args, "script_path");
        if (sp.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'script_path'");
        int64_t max = args_int(ctx.args, "max_results", 100);
        Array out;
        String pattern = sp;
        collect_nodes_by(ctx.root, [pattern](Node *n) {
            Variant sv = n->get("script");
            Ref<Script> script = sv;
            return script.is_valid() && script->get_path().contains(pattern);
        }, out, max, ctx.root);
        Dictionary d; d["matches"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace
