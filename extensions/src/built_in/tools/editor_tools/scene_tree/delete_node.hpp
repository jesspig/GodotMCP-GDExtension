
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/undo_stack.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/time.hpp>

namespace godot_mcp {

class DeleteNodeTool : public ITool {
public:
    String name() const noexcept override { return "delete_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Delete a node and its children from the scene";
    }
    String description() const override {
        return "Deletes the specified node and all its children. Root node deletion is restricted by default. "
               "All changes go through EditorUndoRedoManager and can be restored with Ctrl+Z.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path to delete (empty = root, not allowed)")
            .prop("force", "boolean", "Force delete root node (dangerous: may lose unsaved data)", false)
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        bool force = args_bool(ctx.args, "force", false);

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        if (node == ctx.root && !force) {
            return ToolResult::err("ROOT_DELETE",
                "Cannot delete the scene root node; pass force=true to override");
        }

        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent, cannot be removed through add_child flow");
        }

        String deleted_path = relative_path(ctx.root, node);
        String deleted_type = node->get_class();
        int64_t index = node->get_index();

        auto *ur = begin_undo_action("MCP: Delete " + deleted_type, ctx.root);
        if (!ur) {
            parent->remove_child(node);
            memdelete(node);
        } else {
            ur->add_do_method(parent, "remove_child", node);
            ur->add_undo_method(parent, "add_child", node, true,
                                static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_undo_method(parent, "move_child", node, index);
            ur->add_undo_method(node, "set_owner", ctx.root);

            commit_undo_action(ur);
        }

        // Clear selection
        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
            }
        }

        // Push MCP undo record (reverse = add_node, properties lost — known limitation)
        if (g_undo_manager) {
            String parent_path = relative_path(ctx.root, parent);
            UndoRecord rec;
            rec.tool_name = "delete_node";
            Dictionary fwd;
            fwd["node_path"] = deleted_path;
            rec.forward_args = fwd;
            Dictionary rev;
            rev["parent_path"] = parent_path;
            rev["class_name"] = deleted_type;
            rev["node_name"] = String(deleted_path).get_file();
            rev["index"] = static_cast<int64_t>(index);
            rec.reverse_args = rev;
            rec.timestamp = godot::Time::get_singleton()->get_unix_time_from_system();
            rec.description = String("Delete ") + deleted_path;
            g_undo_manager->push(std::move(rec));
        }

        Dictionary data;
        data["deleted"] = deleted_path;
        data["type"] = deleted_type;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
