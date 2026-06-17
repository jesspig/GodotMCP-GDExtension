// =====================================================================
// commands/cmd_utils.cpp -- Implementation of cmd_utils.hpp helpers.
//
// Every function runs on the main thread (called from EditorPlugin::_process
// via the WebSocket dispatch loop). No locking or thread-safety required.
// =====================================================================

#include "cmd_utils.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

using namespace godot;

namespace godot_mcp {

// ---------------------------------------------------------------------
// Edited-scene helpers
// ---------------------------------------------------------------------

Node *get_root() {
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) {
        return nullptr;
    }
    return ei->get_edited_scene_root();
}

Node *get_root_or_error(Dictionary &out_error) {
    Node *root = get_root();
    if (!root) {
        out_error["error"] = "No scene is currently open in the editor";
    }
    return root;
}

Node *resolve_node(Node *root, const String &path, int depth) {
    if (!root) {
        return nullptr;
    }
    // Empty / canonical-root paths -> root itself.
    if (path.is_empty() || path == "." || path == "/" || path == "/root") {
        return root;
    }
    const String root_name = root->get_name();
    if (path == root_name) {
        return root;
    }
    const String absolute_prefix = String("/root/") + root_name;
    if (path == absolute_prefix) {
        return root;
    }
    // "/root/<name>/..." -> strip the prefix and look up under root.
    const String absolute_child = absolute_prefix + String("/");
    if (path.begins_with(absolute_child)) {
        return root->get_node_or_null(NodePath(path.substr(absolute_child.length())));
    }
    // "<name>/..." -> auto-strip the root name.
    const String relative_prefix = root_name + String("/");
    if (path.begins_with(relative_prefix)) {
        return root->get_node_or_null(NodePath(path.substr(relative_prefix.length())));
    }
    // Any other path is treated as a NodePath relative to root.
    Node *found = root->get_node_or_null(NodePath(path));
    if (found) return found;
    if (depth >= kMaxResolveDepth) return nullptr;
    for (int64_t i = 0; i < root->get_child_count(); i++) {
        Node *c = Object::cast_to<Node>(root->get_child(static_cast<int>(i)));
        if (!c) continue;
        Node *sub = resolve_node(c, path, depth + 1);
        if (sub) return sub;
    }
    return nullptr;
}

EditorUndoRedoManager *get_undo_redo() {
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) {
        return nullptr;
    }
    return ei->get_editor_undo_redo();
}

// ---------------------------------------------------------------------
// Args parsing helpers
// ---------------------------------------------------------------------

String args_string(const Dictionary &args, const String &key, const String &default_value) {
    if (!args.has(key)) {
        return default_value;
    }
    const Variant v = args[key];
    if (v.get_type() == Variant::NIL) {
        return default_value;
    }
    return v.stringify();
}

int64_t args_int(const Dictionary &args, const String &key, int64_t default_value) {
    if (!args.has(key)) {
        return default_value;
    }
    const Variant v = args[key];
    switch (v.get_type()) {
        case Variant::INT:   return static_cast<int64_t>(v);
        case Variant::FLOAT: return static_cast<int64_t>(static_cast<double>(v));
        case Variant::BOOL:  return static_cast<bool>(v) ? 1 : 0;
        default:             return default_value;
    }
}

double args_float(const Dictionary &args, const String &key, double default_value) {
    if (!args.has(key)) {
        return default_value;
    }
    const Variant v = args[key];
    switch (v.get_type()) {
        case Variant::FLOAT: return static_cast<double>(v);
        case Variant::INT:   return static_cast<double>(static_cast<int64_t>(v));
        default:             return default_value;
    }
}

bool args_bool(const Dictionary &args, const String &key, bool default_value) {
    if (!args.has(key)) {
        return default_value;
    }
    const Variant v = args[key];
    if (v.get_type() == Variant::BOOL) {
        return static_cast<bool>(v);
    }
    if (v.get_type() == Variant::INT) {
        return static_cast<int64_t>(v) != 0;
    }
    return default_value;
}

// ---------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------

String relative_path(Node *root, Node *node) {
    if (!root || !node) {
        return String();
    }
    if (root == node) {
        return String(".");
    }
    // Build the path piece by piece walking up the tree.
    Node *cur = node;
    String result;
    while (cur && cur != root) {
        const String name = cur->get_name();
        if (result.is_empty()) {
            result = name;
        } else {
            result = name + String("/") + result;
        }
        cur = cur->get_parent();
    }
    if (cur != root) {
        // node is not actually a descendant of root.
        return String();
    }
    return result;
}

String globalize_path(const String &path) {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    if (!ps) {
        return String();
    }
    return ps->globalize_path(path);
}

bool ensure_parent_dir(const String &res_path) {
    // For "res://file.ext" → no directory component → root exists
    if (res_path.begins_with("res://") && res_path.find("/", 6) < 0) {
        return true;
    }
    if (res_path.begins_with("user://") && res_path.find("/", 7) < 0) {
        return true;
    }
    const int slash = static_cast<int>(res_path.rfind("/"));
    if (slash <= 0) {
        return true;
    }
    const String parent = res_path.substr(0, slash);
    if (DirAccess::dir_exists_absolute(parent)) {
        return true;
    }
    return DirAccess::make_dir_recursive_absolute(parent) == OK;
}

// ---------------------------------------------------------------------
// Editor write helpers
// ---------------------------------------------------------------------

void undoable_set(Node *node,
                  const String &property,
                  const Variant &new_value,
                  const String &action_name) {
    if (!node) {
        return;
    }
    EditorUndoRedoManager *ur = get_undo_redo();
    if (!ur) {
        // Fall back to direct set without undo.
        node->set(property, new_value);
        return;
    }
    const Variant old_value = node->get(property);
    // Apply immediately for instant feedback.
    node->set(property, new_value);
    ur->create_action(action_name);
    ur->add_do_property(node, property, new_value);
    ur->add_undo_property(node, property, old_value);
    ur->commit_action(false);  // do already executed above
}

void mark_scene_dirty() {
    Node *root = get_root();
    if (!root) {
        return;
    }
    EditorUndoRedoManager *ur = get_undo_redo();
    if (!ur) {
        return;
    }
    // Create a no-op action targeting the scene root so the editor flags
    // the scene as having unsaved changes.
    ur->create_action("MCP: mark scene unsaved");
    ur->add_do_method(root, "set_meta", "__mcp_dirty", true);
    ur->add_undo_method(root, "remove_meta", "__mcp_dirty");
    ur->commit_action();
}

void notify_file_changed(const String &path) {
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) {
        return;
    }
    EditorFileSystem *fs = ei->get_resource_filesystem();
    if (fs) {
        fs->update_file(path);
    }
}

}  // namespace godot_mcp
