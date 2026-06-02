// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class GetNodeRotationTool : public ITool {
public:
    String name() const override { return "get_node_rotation"; }
    String category() const override { return "property/2d"; }
    String brief() const override { return "Get a node's rotation in degrees"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Gets the rotation_degrees of a node."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p; p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}(); s["properties"] = p;
        Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        double deg = (double)ctx.node->get("rotation_degrees");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["degrees"] = deg;
        return ToolResult::ok(d);
    }
};

} // namespace
