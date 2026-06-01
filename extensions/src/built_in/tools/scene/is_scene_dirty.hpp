// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

namespace godot_mcp {

class IsSceneDirtyTool : public ITool {
public:
    String name() const override { return "is_scene_dirty"; }
    String category() const override { return "scene"; }
    String brief() const override { return "Check if the current scene has unsaved changes"; }
    String description() const override {
        return "Compares the current undo-redo version against the last recorded save marker. "
               "Returns {dirty: false, hint: \"No scene open\"} when no scene is open.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        schema["properties"] = Dictionary();
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        EditorInterface *ei = EditorInterface::get_singleton();
        Node *root = ei->get_edited_scene_root();
        if (!root) {
            Dictionary data;
            data["dirty"] = false;
            data["hint"] = "No scene open";
            return ToolResult::ok(data);
        }

        EditorUndoRedoManager *ur = ei->get_editor_undo_redo();
        if (!ur) {
            Dictionary data;
            data["dirty"] = false;
            return ToolResult::ok(data);
        }

        int64_t hid = ur->get_object_history_id(root);
        UndoRedo *undo_redo = ur->get_history_undo_redo((uint32_t)hid);
        if (!undo_redo) {
            Dictionary data;
            data["dirty"] = false;
            return ToolResult::ok(data);
        }

        uint64_t ver = undo_redo->get_version();
        int64_t saved = root->get_meta("__mcp_saved_version", 0);

        Dictionary data;
        data["dirty"] = ver > (uint64_t)saved;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
