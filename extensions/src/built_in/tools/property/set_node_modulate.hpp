// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/variant/color.hpp>

namespace godot_mcp {

class SetNodeModulateTool : public ITool {
public:
    String name() const override { return "set_node_modulate"; }
    String category() const override { return "property/2d"; }
    String brief() const override { return "Set a node's modulate color"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the modulate color of a CanvasItem node via r/g/b/a or color hex."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["r"] = [](){Dictionary d;d["type"]="number";d["description"]="Red (0-1)";return d;}();
        p["g"] = [](){Dictionary d;d["type"]="number";d["description"]="Green (0-1)";return d;}();
        p["b"] = [](){Dictionary d;d["type"]="number";d["description"]="Blue (0-1)";return d;}();
        p["a"] = [](){Dictionary d;d["type"]="number";d["description"]="Alpha (0-1)";return d;}();
        p["color"] = [](){Dictionary d;d["type"]="string";d["description"]="CSS hex color (#rrggbb or #rrggbbaa)";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Color c(args_float(ctx.args, "r", 1), args_float(ctx.args, "g", 1), args_float(ctx.args, "b", 1), args_float(ctx.args, "a", 1));
        String color_str = args_string(ctx.args, "color");
        if (!color_str.is_empty()) c = Color::html(color_str);
        undoable_set(ctx.node, "modulate", c, "Set modulate for " + relative_path(ctx.root, ctx.node));
        Color actual = ctx.node->get("modulate");
        Dictionary d; d["node_path"] = relative_path(ctx.root, ctx.node); d["property"] = "modulate";
        d["r"] = actual.r; d["g"] = actual.g; d["b"] = actual.b; d["a"] = actual.a;
        return ToolResult::ok(d);
    }
};

} // namespace
