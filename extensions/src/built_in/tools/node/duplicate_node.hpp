// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class DuplicateNodeTool : public ITool {
public:
    String name() const override { return "duplicate_node"; }
    String category() const override { return "node"; }
    String brief() const override { return "Duplicate a node in the scene"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Creates a copy of a node and adds it as a sibling.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to duplicate"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *n = ctx.node;
        Node *root = ctx.root;
        Node *parent = n->get_parent();
        if (!parent) return ToolResult::err("NO_PARENT", "Node has no parent");

        Node *dup = Object::cast_to<Node>(n->duplicate());
        if (!dup) return ToolResult::err("DUPLICATE_FAILED", "Failed to duplicate node");

        String nm = n->get_name() + String("_copy");
        dup->set_name(nm);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur && parent) {
            ur->create_action("Duplicate Node: " + relative_path(root, n));
            ur->add_do_method(parent, "add_child", Variant(dup));
            ur->add_do_method(dup, "set_owner", Variant(root));
            ur->add_do_reference(dup);
            ur->add_undo_method(parent, "remove_child", Variant(dup));
            ur->commit_action();
        }

        Dictionary data;
        data["new_path"] = relative_path(root, dup);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
