#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp::scene_diff_detail {

using godot::Array;
using godot::Dictionary;
using godot::HashMap;
using godot::Node;
using godot::String;
using godot::Variant;

inline bool should_skip_property(const String &name) {
    if (name == "script") return true;
    if (name == "unique_name_in_owner") return true;
    if (name == "owner") return true;
    if (name == "scene_file_path") return true;
    if (name == "process_mode") return true;
    if (name.begins_with("metadata/")) return true;
    if (name.begins_with("__")) return true;
    return false;
}

inline void collect_by_path(Node *node, HashMap<String, Node*> &map, const String &parent_path) {
    String path = parent_path.is_empty() ? String(node->get_name()) : parent_path + String("/") + node->get_name();
    map[path] = node;
    for (int64_t i = 0; i < node->get_child_count(); i++) {
        Node *child = node->get_child(static_cast<int>(i));
        if (child) {
            collect_by_path(child, map, path);
        }
    }
}

} // namespace godot_mcp::scene_diff_detail
