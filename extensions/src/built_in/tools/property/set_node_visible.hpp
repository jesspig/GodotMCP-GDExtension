// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class SetNodeVisibleTool : public ITool {
public:
    String name() const override { return "set_node_visible"; }
    String category() const override { return "property/2d"; }
    String brief() const override { return "Set a node's visible state"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets whether a node is visible with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["visible"] = [](){Dictionary d;d["type"]="boolean";d["description"]="Visible state";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        bool v = args_bool(ctx.args, "visible", true);
        undoable_set(ctx.node, "visible", v, "Set visible for " + relative_path(ctx.root, ctx.node));
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "visible"; d["visible"] = (bool)ctx.node->get("visible");
        return ToolResult::ok(d);
    }
};

} // namespace
