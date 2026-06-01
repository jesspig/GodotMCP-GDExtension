// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ResetParentTool : public ITool {
public:
    String name() const override { return "reset_parent"; }
    String category() const override { return "node"; }
    String brief() const override { return "Reparent a node to a new parent"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Changes a node's parent while preserving its position in the old parent via undo.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to reparent"; return d; }();
        props["new_parent_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "New parent node path"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); req.push_back("new_parent_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String np = args_string(ctx.args, "new_parent_path");
        Node *root = ctx.root;
        Node *node = ctx.node;
        Node *new_parent = resolve_node(root, np);
        if (!new_parent) return ToolResult::err("PARENT_NOT_FOUND", "New parent not found: " + np);

        Node *old_parent = node->get_parent();
        if (!old_parent) return ToolResult::err("NO_PARENT", "Node has no parent");

        int64_t old_idx = node->get_index();

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Reparent " + relative_path(root, node) + " to " + np);
            ur->add_do_method(old_parent, "remove_child", Variant(node));
            ur->add_do_method(new_parent, "add_child", Variant(node));
            ur->add_do_method(node, "set_owner", Variant(root));
            ur->add_undo_method(new_parent, "remove_child", Variant(node));
            ur->add_undo_method(old_parent, "add_child", Variant(node));
            ur->add_undo_method(old_parent, "move_child", Variant(node), Variant(old_idx));
            ur->add_undo_method(node, "set_owner", Variant(root));
            ur->commit_action();
        }

        Dictionary data;
        data["node_path"] = relative_path(root, node);
        data["new_parent"] = np;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
