#pragma once

#include <c4/substr.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <string>

namespace godot_mcp {

// Recursively convert a ryml NodeRef to a Godot Variant.
//   - MAP nodes  → Dictionary
//   - SEQ nodes  → Array
//   - scalar     → String, float, int, bool, or null Variant
inline godot::Variant ryml_to_variant(ryml::ConstNodeRef node) {
    using namespace godot;

    if (node.is_stream()) return Variant();

    if (node.is_map()) {
        Dictionary dict;
        const auto nch = node.num_children();
        for (size_t i = 0; i < nch; ++i) {
            const auto child = node.child(i);
            if (!child.is_keyval() && !child.has_val()) continue;
            const String key(child.key().data());
            dict[key] = ryml_to_variant(child);
        }
        return dict;
    }

    if (node.is_seq()) {
        Array arr;
        const auto nch = node.num_children();
        for (size_t i = 0; i < nch; ++i) {
            const auto child = node.child(i);
            arr.push_back(ryml_to_variant(child));
        }
        return arr;
    }

    // Scalar
    const c4::csubstr val = node.val();
    if (val.size() == 0 || val == "~") return Variant();

    const std::string s(val.data(), val.size());

    // Bool
    if (val == "true" || val == "yes" || val == "on") return true;
    if (val == "false" || val == "no" || val == "off") return false;

    // Try int
    char *end = nullptr;
    const long long int_val = std::strtoll(s.c_str(), &end, 0);
    if (end != s.c_str() && *end == '\0') {
        return (int64_t)int_val;
    }

    // Try float
    end = nullptr;
    const double float_val = std::strtod(s.c_str(), &end);
    if (end != s.c_str() && *end == '\0') {
        return float_val;
    }

    // Fallback to string
    return String(s.c_str());
}

// Parse YAML string content to a Godot Variant (top-level must be map or seq).
inline godot::Variant parse_yaml(const godot::String &yaml_text) {
    const godot::CharString utf8 = yaml_text.utf8();
    std::string buf(utf8.ptr(), utf8.length());
    ryml::Tree tree = ryml::parse_in_place(c4::to_substr(buf));
    return ryml_to_variant(tree.rootref());
}

} // namespace godot_mcp
