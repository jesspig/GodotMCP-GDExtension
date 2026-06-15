#pragma once
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/array.hpp>
#include <type_traits>

namespace godot_mcp {

template <typename T>
struct always_false_arg_typed : std::false_type {};

template <typename T>
T args_get_typed(const godot::Dictionary &args, const godot::String &key, const T &default_value) {
    if (!args.has(key)) return default_value;
    const godot::Variant v = args[key];

    if constexpr (std::is_same_v<T, godot::String>) {
        return v.get_type() == godot::Variant::STRING
            ? godot::String(v) : default_value;
    } else if constexpr (std::is_same_v<T, godot::Dictionary>) {
        return v.get_type() == godot::Variant::DICTIONARY
            ? godot::Dictionary(v) : default_value;
    } else if constexpr (std::is_same_v<T, godot::Array>) {
        return v.get_type() == godot::Variant::ARRAY
            ? godot::Array(v) : default_value;
    } else if constexpr (std::is_same_v<T, godot::Vector2>) {
        if (v.get_type() == godot::Variant::VECTOR2) return godot::Vector2(v);
        if (v.get_type() == godot::Variant::VECTOR3) {
            auto v3 = godot::Vector3(v);
            return godot::Vector2(v3.x, v3.y);
        }
        return default_value;
    } else if constexpr (std::is_same_v<T, godot::Vector3>) {
        if (v.get_type() == godot::Variant::VECTOR3) return godot::Vector3(v);
        if (v.get_type() == godot::Variant::VECTOR2) {
            auto v2 = godot::Vector2(v);
            return godot::Vector3(v2.x, v2.y, 0.0f);
        }
        return default_value;
    } else if constexpr (std::is_same_v<T, godot::Color>) {
        if (v.get_type() == godot::Variant::COLOR) return godot::Color(v);
        if (v.get_type() == godot::Variant::DICTIONARY) {
            auto d = godot::Dictionary(v);
            return godot::Color(
                static_cast<float>(d.get("r", 1.0)),
                static_cast<float>(d.get("g", 1.0)),
                static_cast<float>(d.get("b", 1.0)),
                static_cast<float>(d.get("a", 1.0))
            );
        }
        return default_value;
    } else if constexpr (std::is_same_v<T, bool>) {
        return v.get_type() == godot::Variant::BOOL
            ? static_cast<bool>(v) : default_value;
    } else if constexpr (std::is_integral_v<T>) {
        if (v.get_type() == godot::Variant::INT) return static_cast<T>(static_cast<int64_t>(v));
        if (v.get_type() == godot::Variant::FLOAT) return static_cast<T>(static_cast<double>(v));
        return default_value;
    } else if constexpr (std::is_floating_point_v<T>) {
        if (v.get_type() == godot::Variant::FLOAT) return static_cast<T>(static_cast<double>(v));
        if (v.get_type() == godot::Variant::INT) return static_cast<T>(static_cast<int64_t>(v));
        return default_value;
    } else {
        static_assert(always_false_arg_typed<T>::value, "args_get_typed: unsupported type T");
        return default_value;
    }
}

} // namespace godot_mcp
