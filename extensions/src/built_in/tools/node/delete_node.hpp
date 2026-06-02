// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class DeleteNodeTool : public ITool {
public:
    String name() const override { return "delete_node"; }
    String category() const override { return "node"; }
    String brief() const override { return "Delete a node from the scene"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Removes a node from the scene. Cannot delete the scene root.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to delete"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *n = ctx.node;
        Node *root = ctx.root;
        if (n == root) return ToolResult::err("CANNOT_DELETE_ROOT", "Cannot delete scene root");

        Node *parent = n->get_parent();
        if (!parent) return ToolResult::err("NO_PARENT", "Node has no parent");

        int64_t idx = n->get_index();
        Node *saved = Object::cast_to<Node>(n->duplicate());
        if (!saved) return ToolResult::err("DUPLICATE_FAILED", "Failed to duplicate node for undo");
        saved->set_owner(root);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Delete Node: " + relative_path(root, n));
            ur->add_undo_method(parent, "add_child", Variant(saved));
            ur->add_undo_method(saved, "set_owner", Variant(root));
            ur->add_undo_method(parent, "move_child", Variant(saved), Variant(idx));
            ur->add_undo_reference(saved);
            ur->commit_action(false);
        }

        parent->remove_child(n);
        n->queue_free();

        Dictionary data;
        data["deleted"] = relative_path(root, n);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
