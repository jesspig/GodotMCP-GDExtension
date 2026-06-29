#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp::scene_diff {

using godot::Error;
using godot::Node;
using godot::Ref;
using godot::PackedScene;

class SceneSnapshot {
public:
    Error capture(Node *root, Ref<PackedScene> &out);
};

} // namespace godot_mcp::scene_diff
