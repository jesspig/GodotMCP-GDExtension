// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

inline void collect_by_type(Node *n, const String &cls, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    if (n->is_class(cls)) { Dictionary d; d["name"] = n->get_name(); d["type"] = n->get_class(); d["path"] = relative_path(root, n); out.append(d); }
    for (int64_t i = 0; i < n->get_child_count(); i++) { Node *c = Object::cast_to<Node>(n->get_child(i)); if (c) collect_by_type(c, cls, out, max, root); }
}

class FindNodesByTypeTool : public ITool {
public:
    String name() const override { return "find_nodes_by_type"; }
    String category() const override { return "find"; }
    String brief() const override { return "Search nodes by class type"; }
    bool needs_scene() const override { return true; }
    String description() const override { return "Finds all nodes that are instances of the given class."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_class"] = [](){Dictionary d;d["type"]="string";d["description"]="Class name (e.g., Sprite2D)";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 100)";return d;}();
        s["properties"] = p; Array r; r.push_back("node_class"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String cls = args_string(ctx.args, "node_class");
        if (cls.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'node_class'");
        int64_t max = args_int(ctx.args, "max_results", 100);
        Array out; collect_by_type(ctx.root, cls, out, max, ctx.root);
        Dictionary d; d["matches"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace
