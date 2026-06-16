
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "scene_tree_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot_mcp {

class DuplicateNodeTool : public ITool {
public:
    String name() const noexcept override { return "duplicate_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Duplicate a node and its children (under the same parent)";
    }
    String description() const override {
        return "Deep copies the node subtree using Node::duplicate(DUPLICATE_USE_INSTANTIATION=15), "
               "inserting the new node as a sibling after the original (-1 = end). "
               "Optionally specify new_name for the copy (default \"<orig>_copy\"). "
               "All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path to duplicate")
            .prop("new_name", "string", "Copy name (empty = <original>_copy)")
            .prop("index", "integer", "Insert position (-1 = after original node)", static_cast<int64_t>(-1))
            .required(Array::make("node_path"))
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String new_name = args_string(ctx.args, "new_name", "");
        int64_t index = args_int(ctx.args, "index", -1);

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent");
        }

        Node *dup = node->duplicate(15);  // DUPLICATE_USE_INSTANTIATION | signals|groups|scripts
        if (!dup) {
            return ToolResult::err("DUPLICATE_FAILED",
                "Node::duplicate returned null");
        }

        if (new_name.is_empty()) {
            new_name = String(node->get_name()) + String("_copy");
        }
        dup->set_name(new_name);

        // resolve insert index: default = right after the source
        int64_t insert_idx = index;
        if (insert_idx < 0) {
            insert_idx = node->get_index() + 1;
        }
        int64_t new_parent_count = parent->get_child_count() + 1;  // includes the new one
        if (insert_idx > new_parent_count) insert_idx = new_parent_count;

        auto *ur = get_undo_redo();
        commit_add_child_undo(ur, "MCP: Duplicate " + node->get_name(), parent, dup, ctx.root, insert_idx);

        // Also set owner outside undo for immediate consistency
        scene_tree_utils::assign_owner_recursive(dup, ctx.root);

        // select the new node
        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(dup);
            }
        }

        Dictionary data;
        data["source"] = relative_path(ctx.root, node);
        data["new_node"] = relative_path(ctx.root, dup);
        data["new_name"] = dup->get_name();
        data["index"] = static_cast<int64_t>(insert_idx);
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
