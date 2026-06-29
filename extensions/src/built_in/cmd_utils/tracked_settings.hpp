#pragma once

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using godot::HashMap;
using godot::String;
using godot::Variant;

namespace godot_mcp {

// Tracks original values of ProjectSettings keys before set_setting modifies them.
// reset_setting reads from this map to restore the original value instead of
// calling ps->clear() (which removes the key from memory and disk entirely).
inline HashMap<String, Variant> &get_setting_overrides() {
    static HashMap<String, Variant> overrides;
    return overrides;
}

} // namespace godot_mcp
