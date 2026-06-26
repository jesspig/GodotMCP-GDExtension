#include "scene_patcher.hpp"

#include "built_in/cmd_utils.hpp"

#include <algorithm>
#include <vector>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/core/memory.hpp>

namespace godot_mcp::scene_diff {

using godot::NodePath;
using godot::UndoRedo;

int ScenePatcher::apply(Node *scene_root, EditorUndoRedoManager *ur, const DiffResult &diff) {
    ur->create_action("MCP: Apply scene changes", UndoRedo::MERGE_DISABLE, scene_root);

    for (int64_t i = 0; i < diff.property_changes.size(); i++) {
        const auto &pc = diff.property_changes[i];
        Node *node = scene_root->get_node<Node>(NodePath(pc.node_path));
        if (!node) continue;
        ur->add_do_property(node, pc.property, pc.new_value);
        ur->add_undo_property(node, pc.property, pc.old_value);
    }

    Vector<NodeChange> deletions;
    Vector<NodeChange> additions;

    for (int64_t i = 0; i < diff.node_changes.size(); i++) {
        const auto &nc = diff.node_changes[i];
        if (nc.type == NodeChange::DELETED) {
            deletions.push_back(nc);
        } else if (nc.type == NodeChange::ADDED) {
            additions.push_back(nc);
        }
    }

    std::vector<NodeChange> sorted_deletions;
    for (int64_t i = 0; i < deletions.size(); i++) {
        sorted_deletions.push_back(deletions[i]);
    }
    std::sort(sorted_deletions.begin(), sorted_deletions.end(),
        [](const NodeChange &a, const NodeChange &b) {
            return a.node_path.length() > b.node_path.length();
        });

    for (const auto &nc : sorted_deletions) {
        Node *node = scene_root->get_node<Node>(NodePath(nc.node_path));
        if (!node) continue;
        Node *parent = node->get_parent();
        if (!parent) continue;
        ur->add_do_method(parent, "remove_child", node);
        ur->add_undo_method(parent, "add_child", node, true);
        ur->add_undo_method(node, "set_owner", scene_root);
    }

    std::vector<NodeChange> sorted_additions;
    for (int64_t i = 0; i < additions.size(); i++) {
        sorted_additions.push_back(additions[i]);
    }
    std::sort(sorted_additions.begin(), sorted_additions.end(),
        [](const NodeChange &a, const NodeChange &b) {
            return a.node_path.length() < b.node_path.length();
        });

    for (const auto &nc : sorted_additions) {
        Node *node = scene_root->get_node<Node>(NodePath(nc.node_path));
        if (!node) continue;
        Node *parent = node->get_parent();
        if (!parent) continue;
        ur->add_do_method(parent, "add_child", node, true);
        ur->add_do_method(node, "set_owner", scene_root);
        ur->add_undo_method(parent, "remove_child", node);
    }

    ur->commit_action();

    return static_cast<int>(diff.property_changes.size() + sorted_deletions.size() + sorted_additions.size());
}

void ScenePatcher::rollback(Node *scene_root, const Ref<PackedScene> &snapshot) {
    if (!scene_root || snapshot.is_null()) return;

    Node *new_root = snapshot->instantiate();
    if (!new_root) return;

    auto *ur = get_undo_redo();
    if (!ur) {
        memdelete(new_root);
        return;
    }

    ur->create_action("MCP: Rollback scene", UndoRedo::MERGE_DISABLE, scene_root);

    Array current_children;
    for (int64_t i = 0; i < scene_root->get_child_count(); i++) {
        Node *child = scene_root->get_child(static_cast<int>(i));
        if (child) {
            current_children.append(child);
            ur->add_do_method(scene_root, "remove_child", child);
            ur->add_undo_method(scene_root, "add_child", child, true);
            ur->add_undo_method(child, "set_owner", scene_root);
        }
    }

    Array snapshot_children;
    for (int64_t i = 0; i < new_root->get_child_count(); i++) {
        Node *child = new_root->get_child(static_cast<int>(i));
        if (child) {
            snapshot_children.append(child);
            ur->add_do_method(scene_root, "add_child", child, true);
            ur->add_do_method(child, "set_owner", scene_root);
            ur->add_undo_method(scene_root, "remove_child", child);
        }
    }

    ur->commit_action();

    for (int64_t i = 0; i < current_children.size(); i++) {
        Node *child = godot::Object::cast_to<Node>(current_children[i]);
        if (child) {
            scene_root->remove_child(child);
        }
    }

    for (int64_t i = 0; i < snapshot_children.size(); i++) {
        Node *child = godot::Object::cast_to<Node>(snapshot_children[i]);
        if (child) {
            scene_root->add_child(child, true);
            child->set_owner(scene_root);
        }
    }

    memdelete(new_root);
}

} // namespace godot_mcp::scene_diff
