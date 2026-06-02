// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class SetNodeRotationTool : public ITool {
public:
    String name() const override { return "set_node_rotation"; }
    String category() const override { return "property/2d"; }
    String brief() const override { return "Set a node's rotation in degrees"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the rotation_degrees of a node with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["degrees"] = [](){Dictionary d;d["type"]="number";d["description"]="Rotation in degrees";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        double deg = args_float(ctx.args, "degrees", 0);
        undoable_set(ctx.node, "rotation_degrees", deg, "Set rotation for " + relative_path(ctx.root, ctx.node));
        double actual = (double)ctx.node->get("rotation_degrees");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "rotation_degrees"; d["degrees"] = actual;
        return ToolResult::ok(d);
    }
};

} // namespace
