// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/property/property_helpers.hpp"

namespace godot_mcp {

class GetNodeCollisionLayerTool : public ITool {
public:
    String name() const override { return "get_node_collision_layer"; }
    String category() const override { return "property"; }
    String brief() const override { return "Get a node's collision_layer"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Gets the collision_layer bitmask of a physics body."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p; p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}(); s["properties"] = p;
        Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Dictionary e = check_collision_compat(ctx.node, relative_path(ctx.root, ctx.node));
        if (e.has("success") && !((bool)e["success"])) return e;
        int64_t v = ctx.node->get("collision_layer");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["collision_layer"] = v;
        return ToolResult::ok(d);
    }
};

} // namespace
