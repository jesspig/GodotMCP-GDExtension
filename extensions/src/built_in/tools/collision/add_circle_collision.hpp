// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/collision/collision_helpers.hpp"

namespace godot_mcp {

class AddCircleCollisionTool : public ITool {
public:
    String name() const override { return "add_circle_collision"; }
    String category() const override { return "collision"; }
    String brief() const override { return "Add a CircleShape2D to a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Adds or sets a CircleShape2D on a CollisionShape2D or physics body."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Target node";return d;}();
        p["radius"] = [](){Dictionary d;d["type"]="number";d["description"]="Circle radius (default: 10)";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        return do_collision_shape(ctx.node, ctx.root, ctx.args, true);
    }
};

} // namespace godot_mcp
