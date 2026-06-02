// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class MoveNodeTool : public ITool {
public:
    String name() const override { return "move_node"; }
    String category() const override { return "node"; }
    String brief() const override { return "Move a node to a new parent"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Moves a node from its current parent to a new parent, preserving undo.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to move"; return d; }();
        props["new_parent_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "New parent node path"; return d; }();
        props["position_index"] = []() { Dictionary d; d["type"] = "integer"; d["description"] = "Optional position index in new parent"; return d; }();
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
        int64_t idx = args_int(ctx.args, "position_index", -1);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Move Node: " + relative_path(root, node));
            ur->add_do_method(old_parent, "remove_child", Variant(node));
            ur->add_do_method(new_parent, "add_child", Variant(node));
            ur->add_do_method(node, "set_owner", Variant(root));
            if (idx >= 0) ur->add_do_method(new_parent, "move_child", Variant(node), Variant(idx));
            ur->add_undo_method(new_parent, "remove_child", Variant(node));
            ur->add_undo_method(old_parent, "add_child", Variant(node));
            ur->add_undo_method(node, "set_owner", Variant(root));
            ur->add_undo_method(old_parent, "move_child", Variant(node), Variant(old_idx));
            ur->commit_action();
        }

        Dictionary data;
        data["new_path"] = relative_path(root, node);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
