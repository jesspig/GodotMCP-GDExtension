#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot_mcp::scene_diff {

using godot::Array;
using godot::Dictionary;
using godot::Node;
using godot::String;
using godot::Variant;
using godot::Vector;

struct PropertyChange {
    String node_path;
    String property;
    Variant old_value;
    Variant new_value;
};

struct NodeChange {
    enum Type : uint8_t {
        ADDED = 0,
        DELETED = 1,
        MODIFIED = 2,
        REPARENTED = 3,
    };
    Type type;
    String node_path;
    Dictionary node_data;
    String old_parent;
    String new_parent;

    Dictionary to_dict() const;
};

struct DiffResult {
    Vector<PropertyChange> property_changes;
    Vector<NodeChange> node_changes;

    int total_changes() const;
    bool has_changes() const;
    Dictionary to_dict() const;
};

class SceneDiff {
public:
    static DiffResult compute(Node *original_root, Node *current_root);
};

} // namespace godot_mcp::scene_diff
