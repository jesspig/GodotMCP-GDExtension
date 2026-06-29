
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/cmd_utils/undo_stack.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/time.hpp>

namespace godot_mcp {

class RenameNodeTool : public ITool {
public:
    String name() const noexcept override { return "rename_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Rename a node in the scene";
    }
    String description() const override {
        return "Changes the name of the specified node. An empty name means no change (invalid operation). "
               "The new name must be unique among siblings under the same parent. All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Node path (empty = scene root)")
            .prop("new_name", "string", "New node name")
            .required(Array::make("new_name"))
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path", "");
        String new_name = args_string(ctx.args, "new_name");

        if (new_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", "new_name cannot be empty");
        }
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        if (node->get_name() == new_name) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["new_name"] = new_name;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        Node *parent = node->get_parent();
        if (parent) {
            Node *conflict = parent->get_node_or_null(String("./") + new_name);
            if (conflict && conflict != node) {
                return ToolResult::err("NAME_CONFLICT",
                    "A node with the same name already exists: " + new_name);
            }
        }

        String old_name = node->get_name();
        auto *ur = begin_undo_action("MCP: Rename " + old_name, ctx.root);
        if (ur) {
            ur->add_do_method(node, "set_name", new_name);
            ur->add_undo_method(node, "set_name", old_name);
            commit_undo_action(ur);
        } else {
            node->set_name(new_name);
        }

        // Push MCP undo record
        if (g_undo_manager) {
            String node_path = relative_path(ctx.root, node);
            UndoRecord rec;
            rec.tool_name = "rename_node";
            Dictionary fwd;
            fwd["node_path"] = node_path;
            fwd["new_name"] = new_name;
            rec.forward_args = fwd;
            Dictionary rev;
            rev["node_path"] = node_path;
            rev["new_name"] = old_name;
            rec.reverse_args = rev;
            rec.timestamp = godot::Time::get_singleton()->get_unix_time_from_system();
            rec.description = String("Rename ") + old_name + String(" -> ") + new_name;
            g_undo_manager->push(std::move(rec));
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["old_name"] = old_name;
        data["new_name"] = new_name;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
