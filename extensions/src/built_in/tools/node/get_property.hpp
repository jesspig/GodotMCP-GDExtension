// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class GetPropertyTool : public ITool {
public:
    String name() const override { return "get_property"; }
    String category() const override { return "node"; }
    String brief() const override { return "Get a property value from a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Reads a specific property from a node and returns its value as JSON.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Absolute or relative node path"; return d; }();
        props["property"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Property name to read"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("property"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String prop = args_string(ctx.args, "property");
        if (prop.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'property'");
        Dictionary data;
        data["value"] = variant_to_json(ctx.node->get(prop));
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
