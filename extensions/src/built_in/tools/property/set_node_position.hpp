// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/vector2.hpp>

namespace godot_mcp {

class SetNodePositionTool : public ITool {
public:
    String name() const override { return "set_node_position"; }
    String category() const override { return "property"; }
    String brief() const override { return "Set a node's 2D position"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the position (x, y) of a node with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["x"] = [](){Dictionary d;d["type"]="number";d["description"]="X position";return d;}();
        p["y"] = [](){Dictionary d;d["type"]="number";d["description"]="Y position";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        double x = args_float(ctx.args, "x", 0), y = args_float(ctx.args, "y", 0);
        undoable_set(ctx.node, "position", Vector2(x, y), "Set position for " + relative_path(ctx.root, ctx.node));
        Vector2 v = ctx.node->get("position");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "position"; d["x"] = v.x; d["y"] = v.y;
        return ToolResult::ok(d);
    }
};

} // namespace
