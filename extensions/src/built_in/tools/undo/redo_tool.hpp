// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

namespace godot_mcp {

class RedoTool : public ITool {
public:
    String name() const override { return "redo"; }
    String category() const override { return "undo"; }
    String brief() const override { return "Redo the last undone action"; }
    String category_description() const override { return String::utf8("操作的撤销与重做"); }
    bool needs_scene() const override { return true; }
    String description() const override { return "Redoes the most recently undone editor action."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        EditorUndoRedoManager *ur = EditorInterface::get_singleton()->get_editor_undo_redo();
        if (!ur) return ToolResult::err("NO_UNDO_REDO", "EditorUndoRedoManager not available");
        int64_t hid = ur->get_object_history_id(ctx.root);
        UndoRedo *undo_redo = ur->get_history_undo_redo((uint32_t)hid);
        if (!undo_redo) return ToolResult::err("NO_HISTORY", "No redo history for current scene");
        if (!undo_redo->has_redo()) return ToolResult::err("NO_ACTION", "Nothing to redo");
        bool ok = undo_redo->redo();
        String name = undo_redo->get_current_action_name();
        Dictionary d; d["action"] = name; d["redo_applied"] = ok; return d;
    }
};

} // namespace godot_mcp
