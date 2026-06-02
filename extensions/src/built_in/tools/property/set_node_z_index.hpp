// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class SetNodeZIndexTool : public ITool {
public:
    String name() const override { return "set_node_z_index"; }
    String category() const override { return "property/2d"; }
    String brief() const override { return "Set a node's z_index"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the z_index of a CanvasItem node with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["z_index"] = [](){Dictionary d;d["type"]="integer";d["description"]="Z index value";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        int64_t z = args_int(ctx.args, "z_index", 0);
        undoable_set(ctx.node, "z_index", z, "Set z_index for " + relative_path(ctx.root, ctx.node));
        int64_t actual = ctx.node->get("z_index");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "z_index"; d["z_index"] = actual;
        return ToolResult::ok(d);
    }
};

} // namespace
