#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class ChangeNodeTypeTool : public ITool {
public:
    String name() const noexcept override { return "change_node_type"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Change a node's type (preserving name, children, and property mapping)";
    }
    String description() const override {
        return "Swaps the node type using Node::replace_by. The new type must inherit from the original type (is_parent_class check). "
               "name left empty = keeps the original name. property_mapping can adjust property values (e.g. if the new type uses different property names). "
               "Note: properties exclusive to the original type will be lost. All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path whose type to change")
            .prop("new_type", "string", "New type (Godot class name)")
            .prop("new_name", "string", "New node name (empty = keep original)")
            .prop("property_mapping", "object", "Property mapping {old_property: new_property} (optional)")
            .required(Array::make("node_path", "new_type"))
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String new_type = args_string(ctx.args, "new_type");
        String new_name = args_string(ctx.args, "new_name", "");
        Variant prop_mapping_var = ctx.args.get("property_mapping", Variant());

        if (new_type.is_empty()) {
            return ToolResult::err("MISSING_ARG", "new_type cannot be empty");
        }
        if (!godot::ClassDB::class_exists(new_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown Godot class: " + new_type);
        }
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        String old_type = node->get_class();
        if (old_type == new_type) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        // check inheritance: new_type must be a subclass of old_type
        // ClassDB::is_parent_class expects (class, parent). It returns true if `class`
        // inherits from `parent`.
        if (!godot::ClassDB::is_parent_class(new_type, old_type) &&
            old_type != new_type) {
            return ToolResult::err("TYPE_INCOMPATIBLE",
                "New type " + new_type +
                " must inherit from " + old_type);
        }

        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent");
        }
        int64_t old_index = node->get_index();

        // Create new instance
        Node *new_node = scene_tree_utils::create_node(new_type, "");
        if (!new_node) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + new_type);
        }
        if (!new_name.is_empty()) {
            new_node->set_name(new_name);
        } else {
            new_node->set_name(node->get_name());
        }

        // apply property mapping if provided
        if (prop_mapping_var.get_type() == godot::Variant::DICTIONARY) {
            godot::Dictionary mapping = prop_mapping_var;
            godot::Array keys = mapping.keys();
            for (int64_t i = 0; i < keys.size(); i++) {
                String old_key = keys[i];
                String new_key = mapping[old_key];
                if (node->has_meta(old_key)) {
                    Variant v = node->get_meta(old_key);
                    new_node->set_meta(new_key, v);
                } else {
                    Variant v = node->get(old_key);
                    if (v.get_type() != godot::Variant::NIL) {
                        new_node->set(new_key, v);
                    }
                }
            }
        }

        auto *ur = begin_undo_action("MCP: Change Type " + old_type + " -> " + new_type, ctx.root);
        if (ur) {
            ur->add_do_method(node, "replace_by", new_node, true);
            ur->add_undo_method(new_node, "replace_by", node, true);

            if (old_index >= 0) {
                ur->add_do_method(parent, "move_child", new_node, static_cast<int64_t>(old_index));
                ur->add_undo_method(parent, "move_child", node, static_cast<int64_t>(old_index));
            }
            commit_undo_action(ur);
        } else {
            node->replace_by(new_node, true);
            if (parent->get_child_count() > 0) {
                int64_t idx = -1;
                for (int64_t i = 0; i < parent->get_child_count(); i++) {
                    if (parent->get_child(i) == new_node) { idx = i; break; }
                }
                if (idx >= 0 && idx != old_index) {
                    parent->move_child(new_node, old_index);
                }
            }
        }

        Dictionary data;
        data["old_type"] = old_type;
        data["new_type"] = new_type;
        data["new_name"] = new_node->get_name();
        data["new_path"] = relative_path(ctx.root, new_node);
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

