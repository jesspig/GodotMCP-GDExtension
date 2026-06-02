// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class GetNodeTextTool : public ITool {
public:
    String name() const override { return "get_node_text"; }
    String category() const override { return "property"; }
    String brief() const override { return "Get a node's text property"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Gets the 'text' property of a node (e.g., Label, Button)."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p; p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}(); s["properties"] = p;
        Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String t = ctx.node->get("text");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["text"] = t;
        return ToolResult::ok(d);
    }
};

} // namespace
