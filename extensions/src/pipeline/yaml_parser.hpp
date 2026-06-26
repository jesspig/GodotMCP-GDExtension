#pragma once

#include <c4/substr.hpp>
#include <ryml.hpp>
#include <ryml_std.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace godot_mcp {

namespace detail {
    inline godot::String to_godot_string(const c4::csubstr &s) {
        std::string tmp(s.data(), s.size());
        return godot::String(tmp.c_str());
    }
    inline constexpr size_t kMaxYamlDepth = 64;
    inline constexpr size_t kMaxYamlChildren = 100000;
}

inline godot::Variant parse_scalar(const c4::csubstr &val) {
    using namespace godot;
    if (val.size() == 0 || val == "~") return Variant();
    const std::string s(val.data(), val.size());
    if (val == "true" || val == "yes" || val == "on") return true;
    if (val == "false" || val == "no" || val == "off") return false;
    int64_t int_val = 0;
    auto [int_ptr, int_ec] = std::from_chars(s.data(), s.data() + s.size(), int_val);
    if (int_ec == std::errc() && int_ptr == s.data() + s.size()) return int_val;
    char *float_end = nullptr;
    double float_val = std::strtod(s.data(), &float_end);
    if (float_end == s.data() + s.size()) return float_val;
    return String(s.c_str());
}

inline godot::Variant ryml_to_variant(ryml::ConstNodeRef node, size_t depth = 0) {
    using namespace godot;
    if (depth > detail::kMaxYamlDepth) {
        Dictionary err; err["error"] = "YAML nesting too deep"; return err;
    }
    if (node.is_stream()) {
        Dictionary err; err["error"] = "YAML stream node not supported"; return err;
    }
    if (node.is_map()) {
        const auto nch = node.num_children();
        if (nch > detail::kMaxYamlChildren) {
            Dictionary err; err["error"] = "YAML map has too many children"; return err;
        }
        Dictionary dict;
        for (size_t i = 0; i < nch; ++i) {
            const auto child = node.child(i);
            const godot::String key = detail::to_godot_string(child.key());
            dict[key] = ryml_to_variant(child, depth + 1);
        }
        return dict;
    }
    if (node.is_seq()) {
        const auto nch = node.num_children();
        if (nch > detail::kMaxYamlChildren) {
            Dictionary err; err["error"] = "YAML sequence has too many children"; return err;
        }
        Array arr;
        for (size_t i = 0; i < nch; ++i) {
            arr.push_back(ryml_to_variant(node.child(i), depth + 1));
        }
        return arr;
    }
    if (node.is_val_quoted()) {
        return godot::String(detail::to_godot_string(node.val()));
    }
    return parse_scalar(node.val());
}

inline void ryml_throw_error(const char *msg, size_t msg_len, ryml::Location, void *) {
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
        Dictionary err; err["error"] = String(e.what()); result = err;
    } catch (...) {
        ryml::set_callbacks(prev); throw;
    }
    ryml::set_callbacks(prev);
    return result;
}

} // namespace godot_mcp
