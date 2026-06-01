// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class SetNodeTextTool : public ITool {
public:
    String name() const override { return "set_node_text"; }
    String category() const override { return "property"; }
    String brief() const override { return "Set a node's text property"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the 'text' property of a node (e.g., Label, Button) with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["text"] = [](){Dictionary d;d["type"]="string";d["description"]="Text content";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String t = args_string(ctx.args, "text");
        if (t.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'text'");
        undoable_set(ctx.node, "text", t, "Set text for " + relative_path(ctx.root, ctx.node));
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "text"; d["text"] = (String)ctx.node->get("text");
        return ToolResult::ok(d);
    }
};

} // namespace
