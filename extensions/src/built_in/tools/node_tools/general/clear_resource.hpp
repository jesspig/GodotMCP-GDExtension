#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include "resource_tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class ClearResourceTool : public ResourceToolBase {
public:
    String name() const noexcept override { return "clear_resource"; }
    String brief() const noexcept override {
        return String("Clear a resource reference");
    }
    String description() const override {
        return String("Clears the resource reference on a node property. "
                            "If the property needs a resource to function, call new_resource afterwards to create a new default instance.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path (empty = root node of current edited scene)")
            .prop("property_name", "string", "Property name (e.g. texture, material)")
            .required({"property_name"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("property_name cannot be empty"));
        }

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        undoable_set(node, prop_name, Variant(),
            String("Clear ") + prop_name + String(" on ") + relative_path(ctx.root, node));

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
