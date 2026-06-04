// =====================================================================
// editor_tools/scene_tree/scene_tree_utils.cpp
// =====================================================================

#include "scene_tree_utils.hpp"

#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace godot_mcp::scene_tree_utils {

// ── Clipboard ──────────────────────────────────────────────────────

static Ref<PackedScene> s_clipboard;

Ref<PackedScene> get_clipboard() {
    return s_clipboard;
}

void set_clipboard(const Ref<PackedScene> &scene) {
    s_clipboard = scene;
}

void clear_clipboard() {
    s_clipboard.unref();
}

bool clipboard_has_content() {
    return s_clipboard.is_valid();
}

// ── Ownership helpers ──────────────────────────────────────────────

void assign_owner_recursive(Node *root, Node *owner) {
    if (!root || !owner) return;
    TypedArray<Node> stack;
    stack.append(root);
    while (stack.size() > 0) {
        Node *node = Object::cast_to<Node>(stack[stack.size() - 1]);
        stack.resize(stack.size() - 1);
        if (node != owner) {
            node->set_owner(owner);
        }
        for (int64_t i = 0; i < node->get_child_count(); i++) {
            Node *c = Object::cast_to<Node>(node->get_child(i));
            if (c) stack.append(c);
        }
    }
}

void clear_owner_recursive(Node *root, Node *stop_at) {
    if (!root) return;
    TypedArray<Node> stack;
    stack.append(root);
    while (stack.size() > 0) {
        Node *node = Object::cast_to<Node>(stack[stack.size() - 1]);
        stack.resize(stack.size() - 1);
        node->set_owner(nullptr);
        if (node == stop_at) continue;
        for (int64_t i = 0; i < node->get_child_count(); i++) {
            Node *c = Object::cast_to<Node>(node->get_child(i));
            if (c) stack.append(c);
        }
    }
}

// ── do_add_child ───────────────────────────────────────────────────

void do_add_child(EditorUndoRedoManager *ur,
                  Node *parent,
                  Node *child,
                  Node *scene_root,
                  int64_t index,
                  const String &action_name) {
    if (!ur || !parent || !child || !scene_root) return;

    ur->create_action(action_name, UndoRedo::MERGE_DISABLE, scene_root);

    if (index < 0 || index >= parent->get_child_count()) {
        ur->add_do_method(parent, "add_child", child, true,
                          (int64_t)Node::INTERNAL_MODE_DISABLED);
    } else {
        ur->add_do_method(parent, "add_child", child, true,
                          (int64_t)Node::INTERNAL_MODE_DISABLED);
        ur->add_do_method(parent, "move_child", child, index);
    }
    ur->add_do_method(child, "set_owner", scene_root);

    // Undo: detach + clear owner
    ur->add_undo_method(parent, "remove_child", child);
    ur->add_undo_method(child, "set_owner", Variant());

    // References
    ur->add_do_reference(child);
    ur->add_undo_reference(child);

    ur->commit_action();
}

// ── Scene serialization ────────────────────────────────────────────

Ref<PackedScene> pack_subtree(Node *node) {
    if (!node) return Ref<PackedScene>();
    Ref<PackedScene> scene;
    scene.instantiate();
    Error err = scene->pack(node);
    if (err != OK) {
        UtilityFunctions::push_error("scene_tree_utils::pack_subtree failed: ", err);
        return Ref<PackedScene>();
    }
    return scene;
}

Node *instantiate_subtree(const Ref<PackedScene> &scene) {
    if (scene.is_null()) return nullptr;
    Node *inst = scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
    return inst;
}

// ── Node creation ──────────────────────────────────────────────────

Node *create_node(const String &class_name, const String &name) {
    if (!ClassDB::class_exists(class_name)) {
        return nullptr;
    }
    Object *obj = ClassDB::instantiate(class_name);
    Node *node = Object::cast_to<Node>(obj);
    if (!node) {
        if (obj) memdelete(obj);
        return nullptr;
    }
    if (!name.is_empty()) {
        node->set_name(name);
    }
    return node;
}

// ── Recursive collection ──────────────────────────────────────────

void collect_node_info(Node *node, Node *root, int64_t max_depth, bool include_scripts, Dictionary &out) {
    if (!node || !root) return;
    Dictionary d;
    d["name"] = node->get_name();
    d["type"] = node->get_class();
    d["path"] = relative_path(root, node);
    d["child_count"] = (int64_t)node->get_child_count();
    d["has_owner"] = node->get_owner() != nullptr;
    if (include_scripts) {
        Ref<Script> s = node->get_script();
        if (s.is_valid()) {
            d["script"] = s->get_path();
        } else {
            d["script"] = Variant();
        }
    }
    out[d["path"]] = d;

    if (max_depth == 0) return;
    int64_t next_depth = max_depth > 0 ? max_depth - 1 : -1;
    for (int64_t i = 0; i < node->get_child_count(); i++) {
        Node *c = Object::cast_to<Node>(node->get_child(i));
        if (c) {
            if (next_depth == 0) {
                Dictionary leaf;
                leaf["name"] = c->get_name();
                leaf["type"] = c->get_class();
                leaf["path"] = relative_path(root, c);
                leaf["child_count"] = (int64_t)c->get_child_count();
                leaf["has_owner"] = c->get_owner() != nullptr;
                leaf["truncated"] = true;
                out[leaf["path"]] = leaf;
                continue;
            }
            collect_node_info(c, root, next_depth, include_scripts, out);
        }
    }
}

// ── Reorder ───────────────────────────────────────────────────────

void reorder_in_parent(Node *child, int64_t new_index) {
    if (!child) return;
    Node *parent = child->get_parent();
    if (!parent) return;
    int64_t cur = child->get_index();
    if (cur == new_index) return;
    parent->move_child(child, new_index);
}

}  // namespace godot_mcp::scene_tree_utils
