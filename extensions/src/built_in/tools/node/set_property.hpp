// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class SetPropertyTool : public ITool {
public:
    String name() const override { return "set_property"; }
    String category() const override { return "node"; }
    String brief() const override { return "Set a property on a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Sets a named property on a node. The value is converted from JSON to Godot Variant.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Absolute or relative node path"; return d; }();
        props["property"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Property name"; return d; }();
        props["value"] = []() { Dictionary d; d["type"] = "object"; d["description"] = "Value to set (converted via json_to_variant)"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("property"); req.push_back("value"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'property'");
        if (!ctx.args.has("value")) return ToolResult::err("MISSING_PARAM", "missing 'value'");

        Variant gv = json_to_variant(ctx.args["value"]);
        undoable_set(ctx.node, prop, gv, "Set " + prop + " for " + relative_path(ctx.root, ctx.node));

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, ctx.node);
        data["property"] = prop;
        data["value"] = variant_to_json(ctx.node->get(prop));
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
