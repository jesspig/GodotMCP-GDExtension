// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class CreateNodeTool : public ITool {
public:
    String name() const override { return "create_node"; }
    String category() const override { return "node"; }
    String brief() const override { return "Create a new node under a parent"; }
    bool needs_scene() const override { return true; }
    String description() const override {
        return "Creates a new node of the specified type under the given parent path.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["parent_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Parent node path"; return d; }();
        props["node_type"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Node class name (e.g., Sprite2D, Node2D)"; return d; }();
        props["name"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Name for the new node"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("parent_path"); req.push_back("node_type"); req.push_back("name"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        const String pp = args_string(ctx.args, "parent_path");
        const String nt = args_string(ctx.args, "node_type");
        const String name = args_string(ctx.args, "name");
        if (name.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'name'");
        if (nt.is_empty()) return ToolResult::err("MISSING_PARAM", "missing 'node_type'");

        Node *root = ctx.root;
        Node *parent = resolve_node(root, pp);
        if (!parent) return ToolResult::err("PARENT_NOT_FOUND", "Parent not found: " + pp);

        Node *node = Object::cast_to<Node>(ClassDB::instantiate(nt));
        if (!node) return ToolResult::err("BAD_TYPE", "Cannot instantiate type: " + nt);
        node->set_name(name);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Create Node: " + name);
            ur->add_do_method(parent, "add_child", Variant(node));
            ur->add_do_method(node, "set_owner", Variant(root));
            ur->add_do_reference(node);
            ur->add_undo_method(parent, "remove_child", Variant(node));
            ur->commit_action();
        }

        Dictionary data;
        data["path"] = relative_path(root, node);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
