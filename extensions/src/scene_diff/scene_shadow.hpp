#pragma once

#include "scene_diff.hpp"
#include "scene_snapshot.hpp"
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot_mcp::scene_diff {

using godot::Dictionary;
using godot::Error;
using godot::HashMap;
using godot::Node;
using godot::Ref;
using godot::PackedScene;
using godot::String;

class SceneShadow {
public:
    Error capture();
    void clear();
    bool has_shadow() const { return has_snapshot_; }
    const String &current_scene_path() const { return scene_path_; }
    DiffResult compute_diff() const;
    int apply();
    void rollback();

private:
    Ref<PackedScene> snapshot_;
    String scene_path_;
    bool has_snapshot_ = false;
    HashMap<String, Dictionary> cached_properties_;
};

SceneShadow &get_global_shadow();

} // namespace godot_mcp::scene_diff
