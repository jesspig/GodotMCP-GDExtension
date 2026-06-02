// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

namespace godot_mcp {

class GetNodePathTool : public ITool {
public:
    String name() const override { return "get_node_path"; }
    String category() const override { return "node"; }
    String brief() const override { return "Get the relative path of a node"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Returns the relative path of a node from the scene root.";
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
        Dictionary data;
        data["path"] = relative_path(ctx.root, ctx.node);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
