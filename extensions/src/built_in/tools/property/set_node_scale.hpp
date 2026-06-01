// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/vector2.hpp>

namespace godot_mcp {

class SetNodeScaleTool : public ITool {
public:
    String name() const override { return "set_node_scale"; }
    String category() const override { return "property"; }
    String brief() const override { return "Set a node's 2D scale"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the scale (x, y) of a node with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["x"] = [](){Dictionary d;d["type"]="number";d["description"]="Scale X";return d;}();
        p["y"] = [](){Dictionary d;d["type"]="number";d["description"]="Scale Y";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        double x = args_float(ctx.args, "x", 1), y = args_float(ctx.args, "y", 1);
        undoable_set(ctx.node, "scale", Vector2(x, y), "Set scale for " + relative_path(ctx.root, ctx.node));
        Vector2 v = ctx.node->get("scale");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "scale"; d["x"] = v.x; d["y"] = v.y;
        return ToolResult::ok(d);
    }
};

} // namespace
