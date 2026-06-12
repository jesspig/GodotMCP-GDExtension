
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class RenameNodeTool : public ITool {
public:
    String name() const override { return "rename_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Rename a node in the scene";
    }
    String description() const override {
        return "Changes the name of the specified node. An empty name means no change (invalid operation). "
               "The new name must be unique among siblings under the same parent. All changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path (empty = scene root)";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "New node name";
            props["new_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("new_name");
        return s;
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
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
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
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Rename " + old_name,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_name", new_name);
            ur->add_undo_method(node, "set_name", old_name);
            ur->commit_action();
        } else {
            node->set_name(new_name);
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
