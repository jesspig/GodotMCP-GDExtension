// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/tools/node/node_helpers.hpp"

namespace godot_mcp {

class GetSceneTreeTool : public ITool {
public:
    String name() const override { return "get_scene_tree"; }
    String category() const override { return "node"; }
    String brief() const override { return "Get the scene tree structure"; }
    bool needs_scene() const override { return true; }
    String description() const override {
        return "Returns a hierarchical tree of all nodes in the current scene, including types, paths, and scripts.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["max_depth"] = []() { Dictionary d; d["type"] = "integer"; d["description"] = "Maximum depth to recurse (default: 10)"; return d; }();
        schema["properties"] = props;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        int64_t max = args_int(ctx.args, "max_depth", 10);
        return serialize_tree(ctx.root, ctx.root, 0, max);
    }
};

} // namespace godot_mcp
