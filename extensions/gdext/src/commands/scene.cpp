// commands/scene.cpp — 16 scene tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

// --- Helper: warn about owner-null nodes ---
Array collect_owner_warnings(Node *root) {
    Array warnings;
    TypedArray<Node> stack; stack.append(root);
    while (stack.size() > 0) {
        Node *node = Object::cast_to<Node>(stack[stack.size() - 1]); stack.resize(stack.size() - 1);
        for (int64_t i = 0; i < node->get_child_count(); i++) {
            Node *c = Object::cast_to<Node>(node->get_child(i)); if (!c) continue;
            if (!c->get_owner() && c != root)
                warnings.append(c->get_name() + String(" (") + c->get_class() + String(")"));
            stack.append(c);
        }
    }
    return warnings;
}

// --- Tools ---
Dictionary cmd_create_scene(const Dictionary &a) {
    String p = args_string(a, "path");
    if (p.is_empty()) return make_error("missing 'path'");
    if (!p.ends_with(".tscn")) return make_error("path must end with .tscn");
    if (FileAccess::file_exists(p)) return make_error("File already exists: " + p);
    String rt = args_string(a, "root_type", "Node"), rn = args_string(a, "root_name", "Root");
    ensure_parent_dir(p);
    Node *root = Object::cast_to<Node>(ClassDB::instantiate(rt));
    if (!root) return make_error("Cannot instantiate root type: " + rt);
    root->set_name(rn);
    Ref<PackedScene> ps; ps.instantiate(); ps->pack(root);
    Error err = ResourceSaver::get_singleton()->save(ps, p);
    if (err != OK) return make_error("Failed to save scene: " + p);
    notify_file_changed(p);
    Dictionary r; r["path"] = p; r["created"] = true; r["root_type"] = rt; r["root_name"] = rn; return r;
}
Dictionary cmd_delete_scene(const Dictionary &a) {
    String p = args_string(a, "path");
    if (p.is_empty()) return make_error("missing 'path'");
    EditorInterface *ei = EditorInterface::get_singleton();
    Array scenes = ei->get_open_scenes();
    bool is_open = false;
    for (int i = 0; i < scenes.size(); i++) { if ((String)scenes[i] == p) { is_open = true; break; } }
    if (is_open) {
        Node *r = ei->get_edited_scene_root();
        if (r && r->get_scene_file_path() != p) { ei->open_scene_from_path(p); }
        if (ei->close_scene() != OK) return make_error("Failed to close scene before deletion");
    }
    Ref<DirAccess> d = DirAccess::open("res://");
    if (d.is_null()) return make_error("Cannot open res://");
    if (d->remove(p) != OK) return make_error("Failed to delete '" + p + "'");
    notify_file_changed(p);
    Dictionary r; r["deleted"] = p; r["was_open"] = is_open; return r;
}
Dictionary cmd_rename_scene(const Dictionary &a) {
    String src = args_string(a, "source_path"), dst = args_string(a, "dest_path");
    EditorInterface *ei = EditorInterface::get_singleton();
    Array scenes = ei->get_open_scenes();
    bool is_open = false;
    for (int i = 0; i < scenes.size(); i++) { if ((String)scenes[i] == src) { is_open = true; break; } }
    if (is_open) {
        Node *r = ei->get_edited_scene_root();
        if (r && r->get_scene_file_path() != src) return make_error("Scene is open but not active tab");
        ei->save_scene(); ei->close_scene();
    }
    Ref<DirAccess> d = DirAccess::open("res://");
    if (d.is_null()) return make_error("Cannot open res://");
    if (d->rename(src, dst) != OK) return make_error("Failed to rename");
    notify_file_changed(dst);
    if (is_open) ei->open_scene_from_path(dst);
    Dictionary r; r["source"] = src; r["destination"] = dst; if (is_open) r["tab_reopened"] = true; return r;
}
Dictionary cmd_branch_to_scene(const Dictionary &a) {
    String p = args_string(a, "node_path"), sp = args_string(a, "scene_path");
    if (sp.is_empty()) return make_error("missing 'scene_path'");
    EditorInterface *ei = EditorInterface::get_singleton();
    Node *root = ei->get_edited_scene_root(); if (!root) return make_error("No scene open");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    if (n == root) return make_error("Cannot branch scene root");
    Node *parent = n->get_parent(); if (!parent) return make_error("Node has no parent");
    int64_t idx = n->get_index();
    // Save branch
    Node *saved = Object::cast_to<Node>(n->duplicate());
    if (!saved) return make_error("Failed to duplicate node");
    Ref<PackedScene> ps; ps.instantiate();
    if (ps->pack(saved) != OK) return make_error("Failed to pack scene");
    ensure_parent_dir(sp);
    if (ResourceSaver::get_singleton()->save(ps, sp) != OK) return make_error("Failed to save scene");
    notify_file_changed(sp);
    // Replace with instance
    Ref<PackedScene> loaded = ResourceLoader::get_singleton()->load(sp);
    if (loaded.is_null()) return make_error("Failed to reload saved scene");
    Node *inst = Object::cast_to<Node>(loaded->instantiate());
    if (!inst) return make_error("Failed to instantiate saved scene");
    inst->set_name(n->get_name());
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) {
        ur->create_action("Branch to Scene: " + sp);
        ur->add_undo_method(parent, "add_child", Array::make(Variant(saved)));
        ur->add_undo_method(saved, "set_owner", Array::make(Variant(root)));
        ur->add_undo_reference(saved);
        ur->commit_action(false);
    }
    parent->remove_child(n); n->queue_free();
    parent->add_child(inst); inst->set_owner(root);
    Dictionary r; r["scene_path"] = sp; r["node_path"] = p; r["replaced_with_instance"] = true; return r;
}
Dictionary cmd_scene_to_branch(const Dictionary &a) {
    String p = args_string(a, "node_path");
    EditorInterface *ei = EditorInterface::get_singleton();
    Node *root = ei->get_edited_scene_root(); if (!root) return make_error("No scene open");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    String sf = n->get_scene_file_path();
    if (sf.is_empty()) return make_error("Node is not an instanced scene; use branch_to_scene");
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) {
        ur->create_action("Make Local");
        ur->add_do_method(n, "set_scene_file_path", Array::make(Variant("")));
        ur->add_undo_method(n, "set_scene_file_path", Array::make(Variant(sf)));
        ur->commit_action();
    }
    n->set_scene_file_path("");
    Dictionary r; r["node_path"] = p; r["converted"] = true; r["source_scene"] = sf; return r;
}
Dictionary cmd_instantiate_scene(const Dictionary &a) {
    String sp = args_string(a, "scene_path"), pp = args_string(a, "parent_path");
    EditorInterface *ei = EditorInterface::get_singleton();
    Node *root = ei->get_edited_scene_root(); if (!root) return make_error("No scene open");
    Node *parent = resolve_node(root, pp); if (!parent) return make_error("parent not found: " + pp);
    Ref<PackedScene> packed = ResourceLoader::get_singleton()->load(sp);
    if (packed.is_null()) return make_error("Scene '" + sp + "' could not be loaded");
    Node *inst = Object::cast_to<Node>(packed->instantiate());
    if (!inst) return make_error("Failed to instantiate scene: " + sp);
    String name = args_string(a, "name");
    if (!name.is_empty()) inst->set_name(name);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) {
        ur->create_action("Instantiate Scene: " + sp);
        ur->add_do_method(parent, "add_child", Array::make(Variant(inst)));
        ur->add_do_method(inst, "set_owner", Array::make(Variant(root)));
        ur->add_do_reference(inst);
        ur->add_undo_method(parent, "remove_child", Array::make(Variant(inst)));
        ur->commit_action();
    }
    Dictionary r; r["path"] = relative_path(root, inst); r["parent"] = relative_path(root, parent); return r;
}
Dictionary cmd_open_scene(const Dictionary &a) {
    String sp = args_string(a, "scene_path");
    if (sp.is_empty()) return make_error("missing 'scene_path'");
    if (!FileAccess::file_exists(sp)) return make_error("Scene file does not exist: " + sp);
    bool inherited = args_bool(a, "set_inherited", false);
    EditorInterface::get_singleton()->open_scene_from_path(sp);
    Dictionary r; r["opened"] = sp; r["loaded"] = true; return r;
}
Dictionary cmd_close_scene(const Dictionary &) {
    EditorInterface *ei = EditorInterface::get_singleton();
    if (ei->close_scene() == OK) { Dictionary r; r["closed"] = true; return r; }
    return make_error("close_scene failed");
}
Dictionary cmd_save_scene(const Dictionary &) {
    EditorInterface *ei = EditorInterface::get_singleton();
    Node *root = ei->get_edited_scene_root();
    Array warnings; if (root) warnings = collect_owner_warnings(root);
    Error err = ei->save_scene();
    if (err != OK) return make_error("save_scene failed; use save_scene_as first");
    String path; if (root) path = root->get_scene_file_path();
    Dictionary r; r["saved"] = path;
    if (warnings.size() > 0) r["warning"] = warnings;
    return r;
}
Dictionary cmd_save_scene_as(const Dictionary &a) {
    String sp = args_string(a, "scene_path");
    if (sp.is_empty()) return make_error("missing 'scene_path'");
    EditorInterface *ei = EditorInterface::get_singleton();
    Node *root = ei->get_edited_scene_root();
    Array warnings; if (root) warnings = collect_owner_warnings(root);
    ei->save_scene_as(sp);
    Dictionary r; r["saved"] = sp;
    if (warnings.size() > 0) r["warning"] = warnings;
    return r;
}
Dictionary cmd_save_all_scenes(const Dictionary &) {
    int64_t before = EditorInterface::get_singleton()->get_open_scenes().size();
    EditorInterface::get_singleton()->save_all_scenes();
    Dictionary r; r["count"] = before; return r;
}
Dictionary cmd_reload_scene(const Dictionary &a) {
    String sp = args_string(a, "scene_path");
    if (sp.is_empty()) return make_error("missing 'scene_path'");
    EditorInterface::get_singleton()->reload_scene_from_path(sp);
    Dictionary r; r["reloaded"] = sp; return r;
}
Dictionary cmd_get_open_scenes(const Dictionary &) {
    Array scenes = EditorInterface::get_singleton()->get_open_scenes();
    Dictionary r; r["scenes"] = scenes; return r;
}
Dictionary cmd_get_open_scene_roots(const Dictionary &) {
    Array roots = EditorInterface::get_singleton()->get_open_scene_roots();
    Array out;
    for (int i = 0; i < roots.size(); i++) {
        Node *rn = Object::cast_to<Node>(roots[i]); if (!rn) continue;
        String sf = rn->get_scene_file_path();
        Dictionary e; e["name"] = rn->get_name(); e["class"] = rn->get_class();
        e["scene_file_path"] = sf.is_empty() ? rn->get_name() : sf;
        out.append(e);
    }
    Dictionary r; r["roots"] = out; return r;
}
Dictionary cmd_mark_scene_unsaved(const Dictionary &) {
    EditorInterface::get_singleton()->mark_scene_as_unsaved();
    Dictionary r; r["marked"] = true; return r;
}
Dictionary cmd_is_scene_dirty(const Dictionary &) {
    EditorInterface *ei = EditorInterface::get_singleton();
    Node *root = ei->get_edited_scene_root();
    if (!root) { Dictionary r; r["dirty"] = false; r["hint"] = "No scene open"; return r; }
    EditorUndoRedoManager *ur = ei->get_editor_undo_redo();
    if (!ur) { Dictionary r; r["dirty"] = false; return r; }
    int64_t hid = ur->get_object_history_id(root);
    UndoRedo *undo_redo = ur->get_history_undo_redo((uint32_t)hid);
    if (!undo_redo) { Dictionary r; r["dirty"] = false; return r; }
    uint64_t ver = undo_redo->get_version();
    int64_t saved = root->get_meta("__mcp_saved_version", 0);
    Dictionary r; r["dirty"] = ver > (uint64_t)saved; return r;
}
} // namespace

void register_scene(HandlerRegistry &reg) {
    reg.register_tool("create_scene", cmd_create_scene);
    reg.register_tool("delete_scene", cmd_delete_scene);
    reg.register_tool("rename_scene", cmd_rename_scene);
    reg.register_tool("branch_to_scene", cmd_branch_to_scene);
    reg.register_tool("scene_to_branch", cmd_scene_to_branch);
    reg.register_tool("instantiate_scene", cmd_instantiate_scene);
    reg.register_tool("open_scene", cmd_open_scene);
    reg.register_tool("close_scene", cmd_close_scene);
    reg.register_tool("save_scene", cmd_save_scene);
    reg.register_tool("save_scene_as", cmd_save_scene_as);
    reg.register_tool("save_all_scenes", cmd_save_all_scenes);
    reg.register_tool("reload_scene", cmd_reload_scene);
    reg.register_tool("get_open_scenes", cmd_get_open_scenes);
    reg.register_tool("get_open_scene_roots", cmd_get_open_scene_roots);
    reg.register_tool("mark_scene_unsaved", cmd_mark_scene_unsaved);
    reg.register_tool("is_scene_dirty", cmd_is_scene_dirty);
}
} // namespace godot_mcp