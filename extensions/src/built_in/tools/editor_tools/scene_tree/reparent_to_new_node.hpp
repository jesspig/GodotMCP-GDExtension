#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class ReparentToNewNodeTool : public ITool {
public:
    String name() const noexcept override { return "reparent_to_new_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Wrap a node in a new parent node";
    }
    String description() const override {
        return "Inserts a new node (of new_class type) at the index position of the original parent, "
               "then moves source_node under it. Equivalent to the editor's \"Reparent to New Node\" operation. "
               "The new parent inherits the owner relationship from the original parent. All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path to wrap")
            .prop("new_class", "string", "New parent node type (Godot class name)")
            .prop("new_name", "string", "New parent node name (empty = type name)")
            .required(Array::make("node_path", "new_class"))
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String new_class = args_string(ctx.args, "new_class");
        String new_name = args_string(ctx.args, "new_name", "");

        if (new_class.is_empty()) {
            return ToolResult::err("MISSING_ARG", "new_class cannot be empty");
        }
        if (!godot::ClassDB::class_exists(new_class)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown Godot class: " + new_class);
        }
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        Node *old_parent = node->get_parent();
        if (!old_parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent");
        }
        if (new_name.is_empty()) new_name = new_class;

        Node *wrapper = scene_tree_utils::create_node(new_class, new_name);
        if (!wrapper) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + new_class);
        }
        MemdeleteGuard<Node> guard(wrapper);
        if (old_parent->has_node(String("./") + new_name)) {
            return ToolResult::err("NAME_CONFLICT",
                "A node with the same name already exists: " + new_name);
        }

        int64_t old_index = node->get_index();

        auto *ur = begin_undo_action("MCP: Reparent to New Node");
        if (ur) {
            ur->add_do_method(old_parent, "add_child", wrapper, true,
                              static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(old_parent, "move_child", wrapper, old_index);
            ur->add_do_method(wrapper, "set_owner", ctx.root);
            ur->add_do_method(old_parent, "remove_child", node);
            ur->add_do_method(wrapper, "add_child", node, true,
                              static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_do_reference(wrapper);
            ur->add_do_reference(node);

            ur->add_undo_method(wrapper, "remove_child", node);
            ur->add_undo_method(old_parent, "add_child", node, true,
                                static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_undo_method(old_parent, "move_child", node, old_index);
            ur->add_undo_method(old_parent, "remove_child", wrapper);
            ur->add_undo_reference(wrapper);
            ur->add_undo_reference(node);

            commit_undo_action(ur);
        } else {
            old_parent->add_child(wrapper, true, godot::Node::INTERNAL_MODE_DISABLED);
            old_parent->move_child(wrapper, old_index);
            wrapper->set_owner(ctx.root);
            old_parent->remove_child(node);
            wrapper->add_child(node, true, godot::Node::INTERNAL_MODE_DISABLED);
        }
        guard.dismiss();

        Dictionary data;
        data["source"] = relative_path(ctx.root, node);
        data["wrapper"] = relative_path(ctx.root, wrapper);
        data["wrapper_type"] = wrapper->get_class();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

