// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/color.hpp>

namespace godot_mcp {

class GetNodeModulateTool : public ITool {
public:
    String name() const override { return "get_node_modulate"; }
    String category() const override { return "property"; }
    String brief() const override { return "Get a node's modulate color"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Gets the modulate (r, g, b, a) of a CanvasItem node."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p; p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}(); s["properties"] = p;
        Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Color c = ctx.node->get("modulate");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["r"] = c.r; d["g"] = c.g; d["b"] = c.b; d["a"] = c.a;
        return ToolResult::ok(d);
    }
};

} // namespace
