#pragma once

#include <c4/substr.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <stdexcept>
#include <string>

namespace godot_mcp {

namespace detail {
    inline godot::String to_godot_string(const c4::csubstr &s) {
        std::string tmp(s.data(), s.size());
        return godot::String(tmp.c_str());
    }
}

// Recursively convert a ryml NodeRef to a Godot Variant.
//   - MAP nodes  → Dictionary
//   - SEQ nodes  → Array
//   - scalar     → String, float, int, bool, or null Variant
inline godot::Variant parse_scalar(const c4::csubstr &val) {
    using namespace godot;
    if (val.size() == 0 || val == "~") return Variant();

    const std::string s(val.data(), val.size());

    if (val == "true" || val == "yes" || val == "on") return true;
    if (val == "false" || val == "no" || val == "off") return false;

    char *end = nullptr;
    const long long int_val = std::strtoll(s.c_str(), &end, 0);
    if (end != s.c_str() && *end == '\0') return static_cast<int64_t>(int_val);

    end = nullptr;
    const double float_val = std::strtod(s.c_str(), &end);
    if (end != s.c_str() && *end == '\0') return float_val;

    return String(s.c_str());
}

// Convert a ryml node to Godot Variant.
// Handles map, seq, and scalar values.
inline godot::Variant ryml_to_variant(ryml::ConstNodeRef node) {
    using namespace godot;

    if (node.is_stream()) {
        Dictionary err;
        err["error"] = "YAML stream node not supported";
        return err;
    }

    if (node.is_map()) {
        Dictionary dict;
        const auto nch = node.num_children();
        for (size_t i = 0; i < nch; ++i) {
            const auto child = node.child(i);
            const godot::String key = detail::to_godot_string(child.key());
            dict[key] = ryml_to_variant(child);
        }
        return dict;
    }

    if (node.is_seq()) {
        Array arr;
        const auto nch = node.num_children();
        for (size_t i = 0; i < nch; ++i) {
            arr.push_back(ryml_to_variant(node.child(i)));
        }
        return arr;
    }

    // Scalar value
    return parse_scalar(node.val());
}

// Parse YAML string content to a Godot Variant (top-level must be map or seq).
inline void ryml_throw_error(const char *msg, size_t msg_len, ryml::Location /*location*/, void * /*user_data*/) {
    throw std::runtime_error(std::string(msg, msg_len));
}

inline godot::Variant parse_yaml(const godot::String &yaml_text) {
    using namespace godot;

    const godot::CharString utf8 = yaml_text.utf8();
    std::string buf(utf8.ptr(), utf8.length());

    ryml::Callbacks prev = ryml::get_callbacks();
    ryml::Callbacks safe = prev;
    safe.m_error = &ryml_throw_error;
    ryml::set_callbacks(safe);

    Variant result;
    try {
        ryml::Tree tree = ryml::parse_in_place(c4::to_substr(buf));
        result = ryml_to_variant(tree.rootref());
    } catch (const std::exception &e) {
        Dictionary err;
        err["error"] = String(e.what());
        result = err;
    }

    ryml::set_callbacks(prev);
    return result;
}

} // namespace godot_mcp
