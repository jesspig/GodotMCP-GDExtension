#pragma once

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

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

inline godot::String compare_variant_fields(
    const godot::Variant &actual,
    const godot::Variant &expected_val,
    const godot::String &type_hint,
    double tolerance)
{
    using namespace godot;

    const String th = normalize_type_hint(type_hint);

    if (th == "vector2" || th == "vector2i") {
        if (actual.get_type() != Variant::VECTOR2 && actual.get_type() != Variant::VECTOR2I) {
            return "is not Vector2";
        }
        const Vector2 actual_v = actual;
        const Array expected_arr = expected_val;
        // Reject a short expected array instead of silently passing. The old
        // `>= 2` guard returned an empty string (test passed) when the fixture
        // was malformed (e.g. [1.0]), hiding broken assertions.
        if (expected_arr.size() < 2) {
            return String("expected array of size >= 2 for Vector2, got size ") +
                   String::num_int64(expected_arr.size());
        }
        if (Math::abs(actual_v.x - static_cast<double>(expected_arr[0])) > tolerance ||
            Math::abs(actual_v.y - static_cast<double>(expected_arr[1])) > tolerance) {
            return String("mismatch: expected (") +
                   String::num(expected_arr[0]) + String(", ") + String::num(expected_arr[1]) +
                   String("), got (") + String::num(actual_v.x) + String(", ") + String::num(actual_v.y) + String(")");
        }
        return String();
    }
    if (th == "vector3" || th == "vector3i") {
        if (actual.get_type() != Variant::VECTOR3 && actual.get_type() != Variant::VECTOR3I) {
            return "is not Vector3";
        }
        const Vector3 actual_v = actual;
        const Array expected_arr = expected_val;
        if (expected_arr.size() < 3) {
            return String("expected array of size >= 3 for Vector3, got size ") +
                   String::num_int64(expected_arr.size());
        }
        if (Math::abs(actual_v.x - static_cast<double>(expected_arr[0])) > tolerance ||
            Math::abs(actual_v.y - static_cast<double>(expected_arr[1])) > tolerance ||
            Math::abs(actual_v.z - static_cast<double>(expected_arr[2])) > tolerance) {
            return "Vector3 mismatch";
        }
        return String();
    }
    if (th == "vector4" || th == "vector4i") {
        if (actual.get_type() != Variant::VECTOR4 && actual.get_type() != Variant::VECTOR4I) {
            return "is not Vector4";
        }
        const Vector4 actual_v = actual;
        const Array expected_arr = expected_val;
        if (expected_arr.size() < 4) {
            return String("expected array of size >= 4 for Vector4, got size ") +
                   String::num_int64(expected_arr.size());
        }
        if (Math::abs(actual_v.x - static_cast<double>(expected_arr[0])) > tolerance ||
            Math::abs(actual_v.y - static_cast<double>(expected_arr[1])) > tolerance ||
            Math::abs(actual_v.z - static_cast<double>(expected_arr[2])) > tolerance ||
            Math::abs(actual_v.w - static_cast<double>(expected_arr[3])) > tolerance) {
            return "Vector4 mismatch";
        }
        return String();
    }
    if (th == "color") {
        if (actual.get_type() != Variant::COLOR) {
            return "is not Color";
        }
        const Color actual_c = actual;
        const Array expected_arr = expected_val;
        if (expected_arr.size() < 3) {
            return String("expected array of size >= 3 for Color, got size ") +
                   String::num_int64(expected_arr.size());
        }
        const double er = expected_arr[0];
        const double eg = expected_arr[1];
        const double eb = expected_arr[2];
        const double ea = expected_arr.size() > 3 ? static_cast<double>(expected_arr[3] ): 1.0;
        if (Math::abs(actual_c.r - er) > tolerance ||
            Math::abs(actual_c.g - eg) > tolerance ||
            Math::abs(actual_c.b - eb) > tolerance ||
            Math::abs(actual_c.a - ea) > tolerance) {
            return "Color mismatch";
        }
        return String();
    }
    if (th == "float" || th == "double") {
        // Guard against implicit Variant coercion masking a real type mismatch.
        // Without this, a String "3.0" could be coerced through Variant and
        // wrongly pass a float assertion.
        const Variant::Type at = actual.get_type();
        const Variant::Type et = expected_val.get_type();
        if (at != Variant::FLOAT && at != Variant::INT) {
            return String("float mismatch: actual is not numeric (type ") + String::num_int64(static_cast<int64_t>(at)) + String(")");
        }
        if (et != Variant::FLOAT && et != Variant::INT) {
            return String("float mismatch: expected is not numeric (type ") + String::num_int64(static_cast<int64_t>(et)) + String(")");
        }
        const double actual_num = actual;
        const double expected_num = expected_val;
        if (Math::abs(actual_num - expected_num) > tolerance) {
            return String("float mismatch: expected ") +
                   String::num(expected_num) + String(", got ") + String::num(actual_num);
        }
        return String();
    }
    if (th == "int" || th == "integer") {
        const Variant::Type at = actual.get_type();
        const Variant::Type et = expected_val.get_type();
        if (at != Variant::INT && at != Variant::FLOAT) {
            return String("int mismatch: actual is not int (type ") + String::num_int64(static_cast<int64_t>(at)) + String(")");
        }
        if (et != Variant::INT) {
            return String("int mismatch: expected is not int (type ") + String::num_int64(static_cast<int64_t>(et)) + String(")");
        }
        const int64_t expected_int = expected_val;
        const int64_t actual_int = actual;
        if (actual_int != expected_int) {
            return String("int mismatch: expected ") +
                   String::num_int64(expected_int) + String(", got ") + String::num_int64(actual_int);
        }
        return String();
    }
    if (th == "bool") {
        if (static_cast<bool>(actual) != static_cast<bool>(expected_val)) {
            return "bool mismatch";
        }
        return String();
    }
    if (th == "string") {
        const String actual_s = actual;
        const String expected_s = expected_val;
        if (actual_s != expected_s) {
            return String("string mismatch: expected '") +
                   expected_s + String("', got '") + actual_s + String("'");
        }
        return String();
    }
    if (actual != expected_val) {
        return String("value mismatch: expected ") +
               JSON::stringify(expected_val) + String(", got ") + JSON::stringify(actual);
    }
    return String();
}

} // namespace godot_mcp
