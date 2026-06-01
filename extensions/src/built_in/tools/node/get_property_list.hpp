// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/node/node_helpers.hpp"

namespace godot_mcp {

class GetPropertyListTool : public ITool {
public:
    String name() const override { return "get_property_list"; }
    String category() const override { return "node"; }
    String brief() const override { return "List all properties of a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Returns the property list for a given node, including type, hint, and hint_string for each property.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Absolute or relative node path"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        return dump_prop_list(ctx.node);
    }
};

} // namespace godot_mcp
