// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

inline void collect_by_group(Node *n, const String &group, Array &out, int64_t max, Node *root) {
    if (out.size() >= max) return;
    if (n->is_in_group(group)) { Dictionary d; d["name"] = n->get_name(); d["type"] = n->get_class(); d["path"] = relative_path(root, n); out.append(d); }
    for (int64_t i = 0; i < n->get_child_count(); i++) { Node *c = Object::cast_to<Node>(n->get_child(i)); if (c) collect_by_group(c, group, out, max, root); }
}

class FindNodesByGroupTool : public ITool {
public:
    String name() const override { return "find_nodes_by_group"; }
    String category() const override { return "find"; }
    String category_description() const override { return String::utf8("按条件搜索场景中的节点"); }
    String brief() const override { return "Search nodes by group membership"; }
    bool needs_scene() const override { return true; }
    String description() const override { return "Finds all nodes belonging to the given group."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["group"] = [](){Dictionary d;d["type"]="string";d["description"]="Group name";return d;}();
        p["max_results"] = [](){Dictionary d;d["type"]="integer";d["description"]="Max results (default: 100)";return d;}();
        s["properties"] = p; Array r; r.push_back("group"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String group = args_string(ctx.args, "group");
        if (group.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'group'");
        int64_t max = args_int(ctx.args, "max_results", 100);
        Array out; collect_by_group(ctx.root, group, out, max, ctx.root);
        Dictionary d; d["matches"] = out; d["count"] = (int64_t)out.size(); d["truncated"] = out.size() >= max;
        return ToolResult::ok(d);
    }
};

} // namespace
