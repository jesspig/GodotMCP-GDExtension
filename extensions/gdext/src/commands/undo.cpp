// commands/undo.cpp — undo/redo tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_undo(const Dictionary &) {
    Node *root = get_root(); if (!root) return make_error("no scene");
    EditorUndoRedoManager *ur = EditorInterface::get_singleton()->get_editor_undo_redo();
    if (!ur) return make_error("EditorUndoRedoManager not available");
    int64_t history_id = ur->get_object_history_id(root);
    UndoRedo *undo_redo = ur->get_history_undo_redo((uint32_t)history_id);
    if (!undo_redo) return make_error("No undo history for current scene");
    if (!undo_redo->has_undo()) { Dictionary r; r["success"] = false; r["hint"] = "Nothing to undo"; return r; }
    bool ok = undo_redo->undo();
    String name = undo_redo->get_current_action_name();
    Dictionary r; r["success"] = ok; r["action"] = name; return r;
}
Dictionary cmd_redo(const Dictionary &) {
    Node *root = get_root(); if (!root) return make_error("no scene");
    EditorUndoRedoManager *ur = EditorInterface::get_singleton()->get_editor_undo_redo();
    if (!ur) return make_error("EditorUndoRedoManager not available");
    int64_t history_id = ur->get_object_history_id(root);
    UndoRedo *undo_redo = ur->get_history_undo_redo((uint32_t)history_id);
    if (!undo_redo) return make_error("No redo history for current scene");
    if (!undo_redo->has_redo()) { Dictionary r; r["success"] = false; r["hint"] = "Nothing to redo"; return r; }
    bool ok = undo_redo->redo();
    String name = undo_redo->get_current_action_name();
    Dictionary r; r["success"] = ok; r["action"] = name; return r;
}
} // namespace

void register_undo(HandlerRegistry &reg) {
    reg.register_tool("undo", cmd_undo);
    reg.register_tool("redo", cmd_redo);
}
} // namespace godot_mcp