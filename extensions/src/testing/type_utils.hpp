#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

constexpr double kDefaultTolerance = 0.0001;

inline godot::String normalize_type_hint(const godot::String &raw) {
    const godot::String lower = raw.to_lower();
    if (lower == "list") return "array";
    if (lower == "dict") return "dictionary";
    if (lower == "number") return "float";
    if (lower == "any") return "";
    return lower;
}

} // namespace godot_mcp
