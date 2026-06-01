// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/property/property_helpers.hpp"

namespace godot_mcp {

class SetNodeCollisionMaskTool : public ITool {
public:
    String name() const override { return "set_node_collision_mask"; }
    String category() const override { return "property"; }
    String brief() const override { return "Set a node's collision_mask"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the collision_mask bitmask of a physics body with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["mask"] = [](){Dictionary d;d["type"]="integer";d["description"]="Mask bitmask value";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Dictionary e = check_collision_compat(ctx.node, relative_path(ctx.root, ctx.node));
        if (e.has("error")) return e;
        int64_t mask = args_int(ctx.args, "mask", 1);
        undoable_set(ctx.node, "collision_mask", mask, "Set collision_mask for " + relative_path(ctx.root, ctx.node));
        int64_t actual = ctx.node->get("collision_mask");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "collision_mask"; d["mask"] = actual;
        return ToolResult::ok(d);
    }
};

} // namespace
