#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot_mcp {

class DuplicateResourceTool : public ITool {
public:
    String name() const override { return "duplicate_resource"; }
    String category() const override { return "node_tools/general"; }
    String brief() const override {
        return String("Duplicate a resource");
    }
    String description() const override {
        return String("Deep-copies the resource on a node property and assigns the duplicate back to the property. "
                            "The copy is disconnected from the original; modifications to the copy will not affect the original.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path (empty = root node of current edited scene)");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Property name (e.g. texture, material)");
            props["property_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("property_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("property_name cannot be empty"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Node not found ") + path);
        }

        Variant val = node->get(prop_name);
        godot::Ref<godot::Resource> res = val;
        if (res.is_null()) {
            return ToolResult::err("NOT_A_RESOURCE",
                String("Property does not currently have a Resource"));
        }

        godot::Ref<godot::Resource> dup = res->duplicate(true);
        if (dup.is_null()) {
            return ToolResult::err("DUPLICATE_FAILED",
                String("Resource duplication failed"));
        }

        undoable_set(node, prop_name, dup,
            String("Duplicate ") + prop_name + String(" on ") + relative_path(ctx.root, node));

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["resource_type"] = dup->get_class();
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
