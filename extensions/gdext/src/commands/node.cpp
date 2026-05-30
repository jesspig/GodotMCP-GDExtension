// commands/node.cpp — 21 node manipulation tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

// --- Helpers ---
String variant_type_name(int64_t tid) {
    switch (tid) {
        case 0: return "NIL"; case 1: return "bool"; case 2: return "int"; case 3: return "float";
        case 4: return "String"; case 5: return "Vector2"; case 6: return "Vector2i";
        case 7: return "Rect2"; case 8: return "Rect2i"; case 9: return "Vector3";
        case 10: return "Vector3i"; case 11: return "Transform2D"; case 12: return "Vector4";
        case 13: return "Vector4i"; case 14: return "Plane"; case 15: return "Quaternion";
        case 16: return "AABB"; case 17: return "Basis"; case 18: return "Transform3D";
        case 19: return "Projection"; case 20: return "Color"; case 21: return "StringName";
        case 22: return "NodePath"; case 23: return "RID"; case 24: return "Object";
        case 25: return "Dictionary"; case 26: return "Array";
        case 27: return "PackedByteArray"; case 28: return "PackedInt32Array";
        case 29: return "PackedInt64Array"; case 30: return "PackedFloat32Array";
        case 31: return "PackedFloat64Array"; case 32: return "PackedStringArray";
        case 33: return "PackedVector2Array"; case 34: return "PackedVector3Array";
        case 35: return "PackedColorArray"; default: return "Unknown";
    }
}

Dictionary serialize_tree(Node *n, Node *root, int64_t depth, int64_t max) {
    Dictionary r; r["name"] = n->get_name(); r["type"] = n->get_class(); r["path"] = relative_path(root, n);
    r["visible"] = (bool)n->get("visible");
    Ref<Script> sc = n->get("script");
    if (sc.is_valid()) r["script"] = sc->get_path(); else r["script"] = Variant();
    Array children;
    if (depth < max) {
        for (int64_t i = 0; i < n->get_child_count(); i++) {
            Node *c = Object::cast_to<Node>(n->get_child(i));
            if (c) children.append(serialize_tree(c, root, depth + 1, max));
        }
    }
    r["children"] = children; r["child_count"] = (int64_t)children.size();
    return r;
}

Dictionary dump_prop_list(Node *n) {
    Array pl = n->get_property_list();
    Array out;
    for (int i = 0; i < pl.size(); i++) {
        Dictionary d = pl[i];
        int64_t usage = (int64_t)d.get("usage", 0);
        if (usage & 0x00000040) continue; // skip groups/categories
        Dictionary e;
        e["name"] = d.get("name", "");
        e["type"] = variant_type_name((int64_t)d.get("type", 0));
        e["hint"] = d.get("hint", 0);
        e["hint_string"] = d.get("hint_string", "");
        out.append(e);
    }
    Dictionary r; r["properties"] = out; return r;
}

// --- Tools ---
Dictionary cmd_get_scene_tree(const Dictionary &a) {
    int64_t max = args_int(a, "max_depth", 10);
    Node *root = get_root(); if (!root) return make_error("no scene");
    return serialize_tree(root, root, 0, max);
}
Dictionary cmd_get_node_path(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary r; r["path"] = relative_path(root, n); return r;
}
Dictionary cmd_get_property_list(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    return dump_prop_list(n);
}
Dictionary cmd_get_property(const Dictionary &a) {
    String p = args_string(a, "node_path"), prop = args_string(a, "property");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary r; r["value"] = variant_to_json(n->get(prop)); return r;
}
Dictionary cmd_create_node(const Dictionary &a) {
    String pp = args_string(a, "parent_path"), nt = args_string(a, "node_type"), name = args_string(a, "name");
    if (name.is_empty()) return make_error("missing 'name'"); if (nt.is_empty()) return make_error("missing 'node_type'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *parent = resolve_node(root, pp); if (!parent) return make_error("parent not found: " + pp);
    Node *node = Object::cast_to<Node>(ClassDB::instantiate(nt));
    if (!node) return make_error("Cannot instantiate type: " + nt);
    node->set_name(name);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Create Node: " + name);
        ur->add_do_method(parent, "add_child", Variant(node));
        ur->add_do_method(node, "set_owner", Variant(root));
        ur->add_do_reference(node);
        ur->add_undo_method(parent, "remove_child", Variant(node));
        ur->commit_action();
    }
    Dictionary r; r["path"] = relative_path(root, node); return r;
}
Dictionary cmd_delete_node(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    if (n == root) return make_error("Cannot delete scene root");
    Node *parent = n->get_parent(); if (!parent) return make_error("Node has no parent");
    int64_t idx = n->get_index();
    Node *saved = Object::cast_to<Node>(n->duplicate());
    if (!saved) return make_error("Failed to duplicate node for undo");
    saved->set_owner(root);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Delete Node: " + p);
        ur->add_undo_method(parent, "add_child", Variant(saved));
        ur->add_undo_method(saved, "set_owner", Variant(root));
        ur->add_undo_method(parent, "move_child", Variant(saved), Variant(idx));
        ur->add_undo_reference(saved);
        ur->commit_action(false);
    }
    parent->remove_child(n);
    n->queue_free();
    Dictionary r; r["deleted"] = p; return r;
}
Dictionary cmd_rename_node(const Dictionary &a) {
    String p = args_string(a, "node_path"), nn = args_string(a, "new_name");
    if (nn.is_empty()) return make_error("missing 'new_name'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    String old = n->get_name();
    n->set_name(nn);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Rename " + old + " to " + nn);
        ur->add_do_property(n, "name", nn); ur->add_undo_property(n, "name", old); ur->commit_action(false); }
    Dictionary r; r["new_path"] = relative_path(root, n); return r;
}
Dictionary cmd_set_property(const Dictionary &a) {
    String p = args_string(a, "node_path"), prop = args_string(a, "property");
    if (!a.has("value")) return make_error("missing 'value'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Variant gv = json_to_variant(a["value"]);
    undoable_set(n, prop, gv, "Set " + prop + " for " + p);
    Dictionary r; r["node_path"] = p; r["property"] = prop; r["value"] = variant_to_json(n->get(prop)); return r;
}
Dictionary cmd_duplicate_node(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Node *parent = n->get_parent();
    Node *dup = Object::cast_to<Node>(n->duplicate());
    if (!dup) return make_error("Failed to duplicate node");
    String nm = n->get_name() + String("_copy"); dup->set_name(nm);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur && parent) { ur->create_action("Duplicate Node: " + p);
        ur->add_do_method(parent, "add_child", Variant(dup));
        ur->add_do_method(dup, "set_owner", Variant(root));
        ur->add_do_reference(dup);
        ur->add_undo_method(parent, "remove_child", Variant(dup));
        ur->commit_action();
    }
    Dictionary r; r["new_path"] = relative_path(root, dup); return r;
}
Dictionary cmd_move_node(const Dictionary &a) {
    String p = args_string(a, "node_path"), np = args_string(a, "new_parent_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *node = resolve_node(root, p); if (!node) return make_error("node not found: " + p);
    Node *new_parent = resolve_node(root, np); if (!new_parent) return make_error("new parent not found: " + np);
    Node *old_parent = node->get_parent(); if (!old_parent) return make_error("node has no parent");
    int64_t old_idx = node->get_index(), idx = args_int(a, "position_index", -1);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Move Node: " + p);
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
    Dictionary r; r["new_path"] = relative_path(root, node); return r;
}
Dictionary cmd_attach_script(const Dictionary &a) {
    String p = args_string(a, "node_path"), sp = args_string(a, "script_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);

    // Force filesystem to process the script file before loading
    EditorFileSystem *fs = EditorInterface::get_singleton()->get_resource_filesystem();
    if (fs) fs->update_file(sp);

    Ref<Script> script = ResourceLoader::get_singleton()->load(sp);
    if (script.is_null()) return make_error("Script '" + sp + "' could not be loaded");

    // Force GDScript to parse and register @export properties
    Ref<GDScript> gdscript = Object::cast_to<GDScript>(script.ptr());
    if (gdscript.is_valid()) {
        const String src = FileAccess::get_file_as_string(sp);
        gdscript->set_source_code(src);
        gdscript->reload();
    }

    Variant old = n->get("script");
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Attach Script: " + sp);
        ur->add_do_property(n, "script", script); ur->add_undo_property(n, "script", old); ur->commit_action(); }
    else n->set("script", script);
    n->notify_property_list_changed();
    Dictionary r; r["node_path"] = p; r["script_path"] = sp; return r;
}
Dictionary cmd_detach_script(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Variant old = n->get("script");
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Detach Script from " + p);
        ur->add_do_property(n, "script", Variant()); ur->add_undo_property(n, "script", old); ur->commit_action(); }
    else n->set("script", Variant());
    Dictionary r; r["node_path"] = p; r["script"] = Variant(); return r;
}
Dictionary cmd_reset_parent(const Dictionary &a) {
    String p = args_string(a, "node_path"), np = args_string(a, "new_parent_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *node = resolve_node(root, p); if (!node) return make_error("node not found: " + p);
    Node *new_parent = resolve_node(root, np); if (!new_parent) return make_error("new parent not found: " + np);
    Node *old_parent = node->get_parent(); if (!old_parent) return make_error("node has no parent");
    int64_t old_idx = node->get_index();
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Reparent " + p + " to " + np);
        ur->add_do_method(old_parent, "remove_child", Variant(node));
        ur->add_do_method(new_parent, "add_child", Variant(node));
        ur->add_do_method(node, "set_owner", Variant(root));
        ur->add_undo_method(new_parent, "remove_child", Variant(node));
        ur->add_undo_method(old_parent, "add_child", Variant(node));
        ur->add_undo_method(old_parent, "move_child", Variant(node), Variant(old_idx));
        ur->add_undo_method(node, "set_owner", Variant(root));
        ur->commit_action();
    }
    Dictionary r; r["node_path"] = relative_path(root, node); r["new_parent"] = np; return r;
}
// Recursively walk descendants of `base`. For any node whose owner == `old_owner`
// (and is not `new_owner`), register `set_owner(new_owner)` via UndoRedo.
// Mirrors Godot editor's `SceneTreeDock::_node_replace_owner`.
static void node_replace_owner(Node *base, Node *old_owner, Node *new_owner,
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

// Find EditorNode in the editor's scene tree. Mirrors Rust crates/gdext find_editor_node().
// EditorNode is not registered as a scripting singleton, so Engine::get_singleton("EditorNode")
// returns nullptr in GDExtension. We must search the scene tree instead.
static Object *find_editor_node(SceneTree *tree) {
    if (!tree) return nullptr;
    Window *root_win = tree->get_root();
    if (!root_win) return nullptr;
    // Window inherits Node, so we can iterate children directly
    int count = root_win->get_child_count();
    for (int i = 0; i < count; i++) {
        Node *child = Object::cast_to<Node>(root_win->get_child(i));
        if (child) {
            String cls = child->get_class();
            if (cls.contains("EditorNode")) {
                return child;
            }
        }
    }
    return nullptr;
}

Dictionary cmd_set_as_root(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *node = resolve_node(root, p); if (!node) return make_error("node not found: " + p);
    if (node == root) { Dictionary r; r["root"] = p; r["hint"] = "Already root"; return r; }
    if (node->get_owner() != root) return make_error("Node must belong to the edited scene to become root");
    String sf = node->get_scene_file_path();
    if (!sf.is_empty()) return make_error("Instantiated scenes can't become root; use scene_to_branch first");
    Node *parent = node->get_parent();
    if (!parent) return make_error("Node has no parent");
    int64_t old_idx = node->get_index();
    String old_root_scene_path = root->get_scene_file_path();

    // Find EditorNode via scene tree (Engine singleton doesn't expose it to GDExtension)
    SceneTree *st = Object::cast_to<SceneTree>(root->get_tree());
    Object *editor_node = find_editor_node(st);

    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) {
        ur->create_action("Make node as Root");

        // ── Do methods (matches Godot scene_tree_dock.cpp TOOL_MAKE_ROOT) ──
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

        // ── Undo methods ──
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

    // Verify the operation succeeded
    Node *new_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (new_root && new_root == node) {
        root->set_name(root->get_name());
        mark_scene_dirty();
        Dictionary r; r["root"] = relative_path(node, node); return r;
    }
    return make_error("Failed to set '" + p + "' as root (EditorNode not found or set_edited_scene failed)");
}
Dictionary cmd_batch_set_property(const Dictionary &a) {
    if (!a.has("node_paths") || a["node_paths"].get_type() != Variant::ARRAY) return make_error("missing 'node_paths' array");
    Array paths = a["node_paths"];
    String prop = args_string(a, "property");
    if (!a.has("value")) return make_error("missing 'value'");
    Variant gv = json_to_variant(a["value"]);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Array results;
    for (int i = 0; i < paths.size(); i++) {
        String pi = (String)paths[i];
        Node *n = resolve_node(root, pi);
        Dictionary e; e["node_path"] = pi;
        if (n) { undoable_set(n, prop, gv, "Batch set " + prop + " for " + pi);
            e["status"] = "ok"; e["value"] = variant_to_json(n->get(prop)); }
        else { e["status"] = "error"; e["message"] = "not found"; }
        results.append(e);
    }
    Dictionary r; r["results"] = results; return r;
}
Dictionary cmd_add_node_to_group(const Dictionary &a) {
    String p = args_string(a, "node_path"), group = args_string(a, "group");
    if (group.is_empty()) return make_error("missing 'group'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    if (n->is_in_group(group)) { Dictionary r; r["node_path"] = p; r["group"] = group; r["hint"] = "Already in group"; return r; }
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Add " + p + " to group " + group);
        ur->add_do_method(n, "add_to_group", Variant(group));
        ur->add_undo_method(n, "remove_from_group", Variant(group));
        ur->commit_action();
    } else n->add_to_group(group);
    Dictionary r; r["node_path"] = p; r["group"] = group; return r;
}
Dictionary cmd_remove_node_from_group(const Dictionary &a) {
    String p = args_string(a, "node_path"), group = args_string(a, "group");
    if (group.is_empty()) return make_error("missing 'group'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    if (!n->is_in_group(group)) { Dictionary r; r["node_path"] = p; r["group"] = group; r["hint"] = "Not in group"; return r; }
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Remove " + p + " from group " + group);
        ur->add_do_method(n, "remove_from_group", Variant(group));
        ur->add_undo_method(n, "add_to_group", Variant(group));
        ur->commit_action();
    } else n->remove_from_group(group);
    Dictionary r; r["node_path"] = p; r["group"] = group; return r;
}
Dictionary cmd_set_node_transform_2d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Vector2 old_pos = n->get("position"); double old_rot = n->get("rotation_degrees"); Vector2 old_scl = n->get("scale");
    double x = args_float(a, "x", old_pos.x), y = args_float(a, "y", old_pos.y);
    double rot = args_float(a, "degrees", old_rot);
    double sx = args_float(a, "scale_x", old_scl.x), sy = args_float(a, "scale_y", old_scl.y);
    Vector2 new_pos(x, y), new_scl(sx, sy);
    undoable_set(n, "position", new_pos, "Set 2D Transform for " + p);
    undoable_set(n, "rotation_degrees", rot, "Set 2D Transform for " + p);
    undoable_set(n, "scale", new_scl, "Set 2D Transform for " + p);
    Dictionary r; r["node_path"] = p; r["x"] = x; r["y"] = y; r["degrees"] = rot; r["scale_x"] = sx; r["scale_y"] = sy; return r;
}
Dictionary cmd_set_node_transform_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    if (!n->is_class("Node3D")) return make_error("Node '" + p + "' is not Node3D");
    Vector3 old_pos = n->get("position"), old_rot = n->get("rotation_degrees"), old_scl = n->get("scale");
    double x = args_float(a, "x", old_pos.x), y = args_float(a, "y", old_pos.y), z = args_float(a, "z", old_pos.z);
    double rx = args_float(a, "rot_x", old_rot.x), ry = args_float(a, "rot_y", old_rot.y), rz = args_float(a, "rot_z", old_rot.z);
    double sx = args_float(a, "scale_x", old_scl.x), sy = args_float(a, "scale_y", old_scl.y), sz = args_float(a, "scale_z", old_scl.z);
    undoable_set(n, "position", Vector3(x, y, z), "Set 3D Transform for " + p);
    undoable_set(n, "rotation_degrees", Vector3(rx, ry, rz), "Set 3D Transform for " + p);
    undoable_set(n, "scale", Vector3(sx, sy, sz), "Set 3D Transform for " + p);
    Dictionary r; r["node_path"] = p; r["x"] = x; r["y"] = y; r["z"] = z; r["rot_x"] = rx; r["rot_y"] = ry; r["rot_z"] = rz; r["scale_x"] = sx; r["scale_y"] = sy; r["scale_z"] = sz; return r;
}
Dictionary cmd_get_node_info(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Ref<Script> sc = n->get("script");
    Array grps = n->get_groups(); Array garr;
    for (int i = 0; i < grps.size(); i++) garr.append(grps[i]);
    Dictionary r; r["name"] = n->get_name(); r["type"] = n->get_class(); r["path"] = relative_path(root, n);
    if (sc.is_valid()) r["script"] = sc->get_path(); else r["script"] = Variant();
    r["visible"] = (bool)n->get("visible"); r["groups"] = garr; r["child_count"] = n->get_child_count();
    r["unique_name"] = n->is_unique_name_in_owner();
    String sf = n->get_scene_file_path(); r["is_instance"] = !sf.is_empty();
    if (!sf.is_empty()) r["scene_file_path"] = sf; else r["scene_file_path"] = Variant();
    Node *owner = n->get_owner();
    if (owner) r["owner"] = relative_path(root, owner); else r["owner"] = Variant();
    return r;
}
Dictionary cmd_get_script_variables(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Array pl = n->get_property_list(); Array out;
    for (int i = 0; i < pl.size(); i++) {
        Dictionary d = pl[i]; int64_t usage = (int64_t)d.get("usage", 0);
        if ((usage & 0x00001000) == 0) continue;
        // Skip groups/categories (usage & 0x00000040 or 0x00000080)
        if (usage & 0x00000040) continue;
        if (usage & 0x00000080) continue;
        String name = d.get("name", ""); int64_t tid = (int64_t)d.get("type", 0);
        Dictionary e; e["name"] = name; e["type"] = variant_type_name(tid); e["value"] = variant_to_json(n->get(name));
        out.append(e);
    }
    Dictionary r; r["node_path"] = p; r["variables"] = out; return r;
}
// Not needed in C++: set_edited_scene_root_impl is handled by EditorInterface
} // namespace

void register_node(HandlerRegistry &reg) {
    reg.register_tool("get_scene_tree", cmd_get_scene_tree);
    reg.register_tool("get_node_path", cmd_get_node_path);
    reg.register_tool("get_property_list", cmd_get_property_list);
    reg.register_tool("get_property", cmd_get_property);
    reg.register_tool("create_node", cmd_create_node);
    reg.register_tool("delete_node", cmd_delete_node);
    reg.register_tool("rename_node", cmd_rename_node);
    reg.register_tool("set_property", cmd_set_property);
    reg.register_tool("duplicate_node", cmd_duplicate_node);
    reg.register_tool("move_node", cmd_move_node);
    reg.register_tool("attach_script", cmd_attach_script);
    reg.register_tool("detach_script", cmd_detach_script);
    reg.register_tool("reset_parent", cmd_reset_parent);
    reg.register_tool("set_as_root", cmd_set_as_root);
    reg.register_tool("batch_set_property", cmd_batch_set_property);
    reg.register_tool("add_node_to_group", cmd_add_node_to_group);
    reg.register_tool("remove_node_from_group", cmd_remove_node_from_group);
    reg.register_tool("set_node_transform_2d", cmd_set_node_transform_2d);
    reg.register_tool("set_node_transform_3d", cmd_set_node_transform_3d);
    reg.register_tool("get_node_info", cmd_get_node_info);
    reg.register_tool("get_script_variables", cmd_get_script_variables);
}
} // namespace godot_mcp