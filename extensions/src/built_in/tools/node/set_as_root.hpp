// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

namespace godot_mcp {

namespace {

void node_replace_owner(Node *base, Node *old_owner, Node *new_owner,
                        EditorUndoRedoManager *ur, bool is_do) {
    for (int64_t i = 0; i < base->get_child_count(); i++) {
        Node *child = Object::cast_to<Node>(base->get_child(i));
        if (!child) continue;
        if (child->get_owner() == old_owner && child != new_owner) {
            if (is_do) ur->add_do_method(child, "set_owner", Variant(new_owner));
            else       ur->add_undo_method(child, "set_owner", Variant(old_owner));
        }
        node_replace_owner(child, old_owner, new_owner, ur, is_do);
    }
}

Object *find_editor_node(SceneTree *tree) {
    if (!tree) return nullptr;
    Window *root_win = tree->get_root();
    if (!root_win) return nullptr;
    int count = root_win->get_child_count();
    for (int i = 0; i < count; i++) {
        Node *child = Object::cast_to<Node>(root_win->get_child(i));
        if (child && child->get_class().contains("EditorNode")) {
            return child;
        }
    }
    return nullptr;
}

} // namespace

class SetAsRootTool : public ITool {
public:
    String name() const override { return "set_as_root"; }
    String category() const override { return "node"; }
    String brief() const override { return "Set a different node as the scene root"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Promotes a child node to become the new scene root. "
               "Supports full undo/redo matching the editor's scene_tree_dock.cpp TOOL_MAKE_ROOT.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node to set as root"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *root = ctx.root;
        Node *node = ctx.node;

        if (node == root) {
            Dictionary data;
            data["root"] = relative_path(root, node);
            data["hint"] = "Already root";
            return ToolResult::ok(data);
        }

        if (node->get_owner() != root)
            return ToolResult::err("OWNER_MISMATCH", "Node must belong to the edited scene to become root");

        String sf = node->get_scene_file_path();
        if (!sf.is_empty())
            return ToolResult::err("INSTANCED", "Instanced scenes can't become root; use scene_to_branch first");

        Node *parent = node->get_parent();
        if (!parent) return ToolResult::err("NO_PARENT", "Node has no parent");

        int64_t old_idx = node->get_index();
        String old_root_scene_path = root->get_scene_file_path();

        SceneTree *st = Object::cast_to<SceneTree>(root->get_tree());
        Object *editor_node = find_editor_node(st);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("Make node as Root");

            ur->add_do_method(parent, "remove_child", Variant(node));
            if (editor_node) {
                ur->add_do_method(editor_node, "set_edited_scene", Variant(node));
            }
            ur->add_do_method(node, "add_child", Variant(root));
            ur->add_do_method(node, "set_scene_file_path", Variant(old_root_scene_path));
            ur->add_do_method(root, "set_scene_file_path", Variant(""));
            ur->add_do_method(node, "set_owner", Variant());
            ur->add_do_method(root, "set_owner", Variant(node));
            ur->add_do_method(node, "set_unique_name_in_owner", Variant(false));
            node_replace_owner(root, root, node, ur, true);

            ur->add_undo_method(root, "set_scene_file_path", Variant(old_root_scene_path));
            ur->add_undo_method(node, "set_scene_file_path", Variant(""));
            ur->add_undo_method(node, "remove_child", Variant(root));
            if (editor_node) {
                ur->add_undo_method(editor_node, "set_edited_scene", Variant(root));
            }
            ur->add_undo_method(parent, "add_child", Variant(node));
            ur->add_undo_method(parent, "move_child", Variant(node), Variant(old_idx));
            ur->add_undo_method(root, "set_owner", Variant());
            ur->add_undo_method(node, "set_owner", Variant(root));
            ur->add_undo_method(node, "set_unique_name_in_owner", Variant(node->is_unique_name_in_owner()));
            node_replace_owner(root, root, root, ur, false);

            ur->commit_action();
        }

        Node *new_root = EditorInterface::get_singleton()->get_edited_scene_root();
        if (new_root && new_root == node) {
            root->set_name(root->get_name());
            mark_scene_dirty();

            Dictionary data;
            data["root"] = relative_path(node, node);
            return ToolResult::ok(data);
        }

        return ToolResult::err("ROOT_FAILED", "Failed to set '" + relative_path(root, node) + "' as root");
    }
};

} // namespace godot_mcp
