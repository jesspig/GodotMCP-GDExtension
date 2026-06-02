// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class FindNodesByNameTool : public ITool {
public:
    String name() const override { return "find_nodes_by_name"; }
    String category() const override { return "find"; }
    String brief() const override { return "Search nodes by name pattern"; }
    bool needs_scene() const override { return true; }
    String description() const override { return "Finds all nodes whose name contains the given pattern."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["pattern"] = [](){Dictionary d;d["type"]="string";d["description"]="Name search pattern";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 100)";return d;}();
        s["properties"] = p; Array r; r.push_back("pattern"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String pattern = args_string(ctx.args, "pattern");
        if (pattern.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'pattern'");
        int64_t max = args_int(ctx.args, "max_results", 100);
        Array out;
        String p = pattern;
        collect_nodes_by(ctx.root, [p](Node *n) { return n->get_name().contains(p); }, out, max, ctx.root);
        Dictionary d; d["matches"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace
