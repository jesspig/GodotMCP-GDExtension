// =====================================================================
// commands/cmd_utils.hpp — Shared helpers for every command handler.
//
// Mirrors the public surface of crates/gdext/src/commands/mod.rs:
//   * resolve_node()             — flexible path resolution
//   * get_root() / get_undo_redo() — edited-scene accessors
//   * variant_to_json() / json_to_variant()
//                                  — j2v/v2j equivalents using Godot's
//                                    native Variant + Dictionary types
//   * undoable_set()             — "apply now + register undo" idiom
//   * ensure_parent_dir()        — mkdir -p for res:// paths
//   * relative_path()            — node path relative to scene root
//   * globalize_path()           — res:// -> absolute disk path
//
// All functions in this header MUST be called from the main thread.
// =====================================================================

#pragma once

#include <functional>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

// ---------------------------------------------------------------------
// Edited-scene helpers
// ---------------------------------------------------------------------

// Returns the root node of the currently-edited scene, or nullptr if the
// editor has no scene open. Callers that need a JSON error response should
// use get_root_or_error().
godot::Node *get_root();

// Like get_root() but writes a uniform `{"error": "no scene open"}` payload
// into `out_error` and returns nullptr on failure.
godot::Node *get_root_or_error(godot::Dictionary &out_error);

// Resolves a textual path against the edited-scene root. Accepts:
//   "", ".", "/", "/root"             -> root itself
//   <root_name>                       -> root itself
//   "/root/<root_name>/Child"         -> Child under root
//   "<root_name>/Child"               -> Child under root (auto-stripped)
//   any other NodePath-compatible str -> root->get_node_or_null(path)
godot::Node *resolve_node(godot::Node *root, const godot::String &path);

// Returns the EditorUndoRedoManager via EditorInterface. Never null when
// the editor is running.
godot::EditorUndoRedoManager *get_undo_redo();

// ---------------------------------------------------------------------
// JSON <-> Variant conversion (Rust's j2v / v2j equivalents)
// ---------------------------------------------------------------------

// Convert a Godot Variant into a JSON-friendly Variant for stringify().
// Recursively expands Vector2/3/4, Color, Rect2, Quaternion, Resource paths,
// Dictionaries and Arrays into plain JSON containers.
godot::Variant variant_to_json(const godot::Variant &v);

// Convert a JSON-parsed Variant back into the most specific Godot type:
//   {x,y}            -> Vector2
//   {x,y,z}          -> Vector3
//   {x,y,z,w}        -> Vector4 / Quaternion (depending on caller intent)
//   {r,g,b[,a]}      -> Color
//   {position,size}  -> Rect2
//   {resource_path}  -> ResourceLoader::load()
//   [a,b]/[a,b,c]/[a,b,c,d] -> Vector2/3 or Color shortcut
//   "res://..."      -> ResourceLoader::load()
//   primitives       -> passed through unchanged
godot::Variant json_to_variant(const godot::Variant &jv);

// ---------------------------------------------------------------------
// Editor write helpers
// ---------------------------------------------------------------------

// Apply `new_value` to `node[property]` immediately AND register a proper
// undo step with the EditorUndoRedoManager. Mirrors Rust's undoable_set().
void undoable_set(godot::Node *node,
                  const godot::String &property,
                  const godot::Variant &new_value,
                  const godot::String &action_name);

// Mark the currently-edited scene as dirty so the editor shows the
// unsaved-changes indicator on its tab.
void mark_scene_dirty();

// Notify EditorFileSystem that `path` (a res:// or absolute path) changed
// on disk. Safe to call from any tool that writes script/scene files.
void notify_file_changed(const godot::String &path);

// Record the current undo-redo version as the "saved" marker on a node.
// Used by scene tools to track whether a scene is dirty.
void save_version_marker(godot::Node *root);

// Collect names of nodes under `root` that have no owner (potential issue).
// Returns an Array of warning strings.
godot::Array collect_owner_warnings(godot::Node *root);

// ---------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------

// Compute a path string for `node` that is RELATIVE TO the scene root, e.g.
// "Pong/Ball" (rather than "/root/Pong/Ball"). Returns "" if node == root.
godot::String relative_path(godot::Node *root, godot::Node *node);

// Convert res:// or user:// to an absolute disk path via ProjectSettings.
// Returns an empty string if globalisation failed.
godot::String globalize_path(const godot::String &path);

// Create any missing parent directories for a res:// path.
// Returns true on success.
bool ensure_parent_dir(const godot::String &res_path);

// ---------------------------------------------------------------------
// Args parsing helpers
// ---------------------------------------------------------------------

// Read a String argument from a Dictionary with a default when missing.
godot::String args_string(const godot::Dictionary &args,
                          const godot::String &key,
                          const godot::String &default_value = "");

// Read an integer argument with a default.
int64_t args_int(const godot::Dictionary &args,
                 const godot::String &key,
                 int64_t default_value = 0);

// Read a float argument with a default.
double args_float(const godot::Dictionary &args,
                  const godot::String &key,
                  double default_value = 0.0);

// Read a boolean argument with a default.
bool args_bool(const godot::Dictionary &args,
               const godot::String &key,
               bool default_value = false);

// ---------------------------------------------------------------------
// Response builders
// ---------------------------------------------------------------------

godot::String json_stringify_safe(const godot::Variant &v);

// ---------------------------------------------------------------------
// Tree traversal
// ---------------------------------------------------------------------

inline void collect_nodes_by(godot::Node *node,
                               std::function<bool(godot::Node *)> predicate,
                               godot::Array &out, int64_t max_results,
                               godot::Node *root) {
    if (out.size() >= max_results) return;
    if (predicate(node)) {
        godot::Dictionary d;
        d["name"] = node->get_name();
        d["type"] = node->get_class();
        d["path"] = relative_path(root, node);
        out.append(d);
    }
    for (int64_t i = 0; i < node->get_child_count(); i++) {
        godot::Node *c = godot::Object::cast_to<godot::Node>(node->get_child(i));
        if (c) collect_nodes_by(c, predicate, out, max_results, root);
    }
}

// ---------------------------------------------------------------------
// Directory traversal
// ---------------------------------------------------------------------

inline void walk_project_dir(const godot::String &dir, const godot::Array &extensions,
                               bool include_addons, int64_t max_results, godot::Array &out) {
    if (out.size() >= max_results) return;
    godot::Ref<godot::DirAccess> d = godot::DirAccess::open(dir);
    if (d.is_null()) return;
    d->list_dir_begin();
    while (true) {
        godot::String n = d->get_next();
        if (n.is_empty()) break;
        if (n == "." || n == "..") continue;
        godot::String full = dir == "res://" ? godot::String("res://") + n : dir + godot::String("/") + n;
        if (d->current_is_dir()) {
            if (!include_addons && (n == "addons" || n == ".godot" || n == ".import")) continue;
            walk_project_dir(full, extensions, include_addons, max_results, out);
        } else {
            bool match = extensions.size() == 0;
            for (int i = 0; i < extensions.size() && !match; i++) {
                if (n.ends_with(godot::String(extensions[i]))) match = true;
            }
            if (match && out.size() < max_results) out.append(full);
        }
    }
}

}  // namespace godot_mcp
