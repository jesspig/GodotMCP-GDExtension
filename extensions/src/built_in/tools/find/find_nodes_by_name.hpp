// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

inline void collect_by_name(Node *n, const String &p, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    if (n->get_name().contains(p)) { Dictionary d; d["name"] = n->get_name(); d["type"] = n->get_class(); d["path"] = relative_path(root, n); out.append(d); }
    for (int64_t i = 0; i < n->get_child_count(); i++) { Node *c = Object::cast_to<Node>(n->get_child(i)); if (c) collect_by_name(c, p, out, max, root); }
}

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
        Array out; collect_by_name(ctx.root, pattern, out, max, ctx.root);
        Dictionary d; d["matches"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace
