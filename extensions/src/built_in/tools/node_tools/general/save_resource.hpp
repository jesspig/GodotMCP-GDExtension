#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include "resource_tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

namespace godot_mcp {

class SaveResourceTool : public ResourceToolBase {
public:
    String name() const noexcept override { return "save_resource"; }
    String brief() const noexcept override {
        return String("Save a resource to file");
    }
    String description() const override {
        return String("Saves the resource on a node property to a file path. Supports .tres (text) and .res (binary) formats.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path (empty = root node of current edited scene)")
            .prop("property_name", "string", "Property name (e.g. texture, material)")
            .prop("save_path", "string", "Save path (res:// or user:// prefix, e.g. res://assets/my_resource.tres)")
            .required({"property_name", "save_path"})
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String prop_name = args_string(ctx.args, "property_name");
        String save_path = args_string(ctx.args, "save_path");

        if (prop_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("property_name cannot be empty"));
        }
        if (save_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("save_path cannot be empty"));
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

        ensure_parent_dir(save_path);
        Error err = godot::ResourceSaver::get_singleton()->save(res, save_path);
        if (err != godot::OK) {
            return ToolResult::err("SAVE_FAILED",
                String("Save failed, error: ") + godot::itos(err));
        }

        notify_file_changed(save_path);

        Dictionary data;
        data["node_path"] = relative_path(ctx.root, node);
        data["property_name"] = prop_name;
        data["save_path"] = save_path;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
