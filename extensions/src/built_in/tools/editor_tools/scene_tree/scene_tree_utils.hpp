// =====================================================================
// editor_tools/scene_tree/scene_tree_utils.hpp
//
// Shared helpers for scene tree CRUD tools. All scene-modifying tools
// commit actions through EditorUndoRedoManager (via cmd_utils::get_undo_redo()),
// mirroring Godot 4.6 SceneTreeDock's undo strategy 1:1.
//
// Key conventions
// --------------
// * add_do_method(parent, "add_child", child, true, InternalMode::INTERNAL_MODE_DISABLED)
//                             — use force_read_only=false; ownership is
//                               established separately via set_owner.
// * create_action() 3rd arg  — must be a node inside the edited scene so
//                               Godot routes the action to that scene's
//                               UndoRedo history slot.
// =====================================================================

#pragma once

#include <optional>

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "built_in/cmd_utils.hpp"
#include "built_in/tool_base.hpp"

namespace godot_mcp::scene_tree_utils {

using godot::Dictionary;
using godot::EditorUndoRedoManager;
using godot::Node;
using godot::PackedScene;
using godot::Ref;
using godot::String;
using godot::TypedArray;
using godot::Variant;

// -- Clipboard (PackedScene-backed) ---------------------------------
// Godot's internal SceneTreeDock::node_clipboard is private; we expose
// a PackedScene-backed clipboard that survives across MCP tool calls
// (Godot process lifetime) and serializes the subtree as a .tscn blob.

Ref<PackedScene> get_clipboard();
void set_clipboard(const Ref<PackedScene> &scene);


// -- Ownership helpers -----------------------------------------------
// Godot saves a node to the .tscn iff its `owner` is set to the edited
// scene root. New nodes must be assigned an owner to become persistent.

void assign_owner_recursive(Node *root, Node *owner);

// -- Undo/Redo helpers ------------------------------------------------
// Wrap a typical "add child + assign owner + update selection" sequence.

void do_add_child(EditorUndoRedoManager *ur,
                  Node *parent,
                  Node *child,
                  Node *scene_root,
                  int64_t index = -1,
                  const String &action_name = "MCP: add child");

// -- Scene serialization helpers -------------------------------------

// Pack a node subtree into a PackedScene. Caller owns the result.
Ref<PackedScene> pack_subtree(Node *node);

// Instantiate the packed scene as a new node under `parent` (without
// auto-add — caller is responsible for add_child + owner wiring).
Node *instantiate_subtree(const Ref<PackedScene> &scene);

// -- Node creation helper --------------------------------------------

// Create a new node of the given class, set its name, and return it.
// Returns nullptr if the class is unknown.
Node *create_node(const String &class_name, const String &name);

// -- Recursive node collection ---------------------------------------

struct NodeInfo {
    String name;
    String type;
    String path;
    int64_t child_count = 0;
    bool has_owner = false;
    String script_path; // empty if no script
};

void collect_node_info(Node *node, Node *root, int64_t max_depth, bool include_scripts, Dictionary &out);

// -- Misc utilities --------------------------------------------------

// Resolve a node path and return a ToolResult-compatible error dictionary
// on failure. Returns std::nullopt on success (out_node is set).
inline std::optional<godot::Dictionary> resolve_node_or_error(
    godot::Node *root, const godot::String &path, godot::Node *&out_node) {
    out_node = godot_mcp::resolve_node(root, path);
    if (!out_node) {
        return ToolResult::err("NODE_NOT_FOUND",
            godot::String("Node not found: ") + path);
    }
    return {};
}

}  // namespace godot_mcp::scene_tree_utils
