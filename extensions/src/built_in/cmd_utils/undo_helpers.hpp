#pragma once
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

inline void commit_add_child_undo(
    godot::EditorUndoRedoManager *ur,
    const godot::String &action_name,
    godot::Node *parent,
    godot::Node *child,
    godot::Node *scene_root,
    int64_t index = -1,
    bool clear_owner_on_undo = true)
{
    if (!ur) {
        parent->add_child(child, true, godot::Node::INTERNAL_MODE_DISABLED);
        child->set_owner(scene_root);
        return;
    }

    ur->create_action(action_name, godot::UndoRedo::MERGE_DISABLE, scene_root);
    if (index >= 0) {
        ur->add_do_method(parent, "add_child", child, true,
            static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED), index);
    } else {
        ur->add_do_method(parent, "add_child", child, true,
            static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
    }
    ur->add_do_method(child, "set_owner", scene_root);
    ur->add_undo_method(parent, "remove_child", child);
    if (clear_owner_on_undo) {
        ur->add_undo_method(child, "set_owner", godot::Variant());
    }

    ur->commit_action();
}

inline void select_single_node(godot::Node *node) {
    auto *ei = godot::EditorInterface::get_singleton();
    if (!ei) return;
    auto *sel = ei->get_selection();
    if (!sel) return;
    sel->clear();
    sel->add_node(node);
}

} // namespace godot_mcp
