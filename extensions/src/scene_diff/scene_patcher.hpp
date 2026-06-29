#pragma once

#include "scene_diff.hpp"
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

namespace godot_mcp::scene_diff {

using godot::EditorUndoRedoManager;
using godot::Node;
using godot::Ref;
using godot::PackedScene;

class ScenePatcher {
public:
    int apply(Node *scene_root, EditorUndoRedoManager *ur, const DiffResult &diff);
    void rollback(Node *scene_root, const Ref<PackedScene> &snapshot);
};

} // namespace godot_mcp::scene_diff
