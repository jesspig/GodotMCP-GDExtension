#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/json.hpp>

namespace godot_mcp {

// Run all assertions against a tool execution result.
//   expect: the "expect" block from YAML config
//   result: the Dictionary returned by HandlerRegistry::execute()
// Returns empty String on pass, or error message on failure.
inline godot::String run_assertions(const godot::Dictionary &expect,
                                    const godot::Dictionary &result) {
    using namespace godot;

    // 1. status check
    if (expect.has("status")) {
        const String expected_status = expect["status"];
        if (expected_status == "success") {
            if (result.has("error")) {
                return String("Expected success, got error: ") + JSON::stringify(result);
            }
        } else if (expected_status == "error") {
            if (!result.has("error")) {
                return String("Expected error, got success: ") + JSON::stringify(result);
            }
        }
    }

    // 2. has_keys check
    if (expect.has("has_keys")) {
        const Array keys = expect["has_keys"];
        for (int i = 0; i < keys.size(); ++i) {
            const String key = keys[i];
            if (!result.has(key)) {
                return String("Missing expected key: ") + key;
            }
        }
    }

    // 3. field_checks: array of {path or key, expect or value, type, tolerance?, not_empty?}
    if (expect.has("field_checks")) {
        const Array checks = expect["field_checks"];
        for (int i = 0; i < checks.size(); ++i) {
            const Dictionary check = checks[i];
            // Accept both "path" (C++ format) and "key" (old Python format)
            const String path = check.has("path") ? check["path"].operator String() : check.get("key", "");
            // Accept both "expect" (C++ format) and "value" (old Python format)
            const Variant expected_val = check.has("expect") ? check["expect"] : check.get("value", Variant());
            String type_hint = check.get("type", "");
            // Normalize type hint: lowercase + alias mapping for backward compat
            {
                const String th_lower = type_hint.to_lower();
                if (th_lower == "list") type_hint = "Array";
                else if (th_lower == "dict") type_hint = "Dictionary";
                else if (th_lower == "number") type_hint = "float";
                else if (th_lower == "any") type_hint = "";
            }
            const bool not_empty = check.get("not_empty", false);

            // Navigate to the field via dot-separated path
            Variant actual = result;
            if (!path.is_empty()) {
                const Array parts = path.split(".");
                for (int p = 0; p < parts.size(); ++p) {
                    if (actual.get_type() == Variant::DICTIONARY) {
                        Dictionary d = actual;
                        actual = d.get(parts[p], Variant());
                    } else {
                        return String("Field path '") + path + String("' cannot be traversed at '") + String(parts[p]) + String("'");
                    }
                }
            }

            // not_empty check
            if (not_empty) {
                if (actual.get_type() == Variant::NIL) {
                    return String("Field '") + path + String("' is nil, expected non-empty");
                }
                if (actual.get_type() == Variant::STRING && ((String)actual).is_empty()) {
                    return String("Field '") + path + String("' is empty string");
                }
                if (actual.get_type() == Variant::ARRAY && ((Array)actual).is_empty()) {
                    return String("Field '") + path + String("' is empty array");
                }
                if (actual.get_type() == Variant::DICTIONARY && ((Dictionary)actual).is_empty()) {
                    return String("Field '") + path + String("' is empty dict");
                }
            }

            if (!check.has("expect") && !check.has("value")) {
                // No value to compare — only did not_empty/type checks above
                continue;
            }

            if (actual.get_type() == Variant::NIL && expected_val.get_type() != Variant::NIL) {
                return String("Field '") + path + String("' is nil, expected value");
            }

            // Compare based on type
            if (type_hint == "Vector2" || type_hint == "Vector2i") {
                if (actual.get_type() != Variant::VECTOR2 && actual.get_type() != Variant::VECTOR2I) {
                    return String("Field '") + path + String("' is not Vector2");
                }
                const Vector2 actual_v = actual;
                const Array expected_arr = expected_val;
                const double tolerance = check.get("tolerance", 0.0001);
                if (expected_arr.size() >= 2) {
                    if (Math::abs(actual_v.x - (double)expected_arr[0]) > tolerance ||
                        Math::abs(actual_v.y - (double)expected_arr[1]) > tolerance) {
                        return String("Field '") + path + String("' mismatch: expected (") +
                               String::num(expected_arr[0]) + String(", ") + String::num(expected_arr[1]) +
                               String("), got (") + String::num(actual_v.x) + String(", ") + String::num(actual_v.y) + String(")");
                    }
                }
            } else if (type_hint == "Vector3" || type_hint == "Vector3i") {
                if (actual.get_type() != Variant::VECTOR3 && actual.get_type() != Variant::VECTOR3I) {
                    return String("Field '") + path + String("' is not Vector3");
                }
                const Vector3 actual_v = actual;
                const Array expected_arr = expected_val;
                const double tolerance = check.get("tolerance", 0.0001);
                if (expected_arr.size() >= 3) {
                    if (Math::abs(actual_v.x - (double)expected_arr[0]) > tolerance ||
                        Math::abs(actual_v.y - (double)expected_arr[1]) > tolerance ||
                        Math::abs(actual_v.z - (double)expected_arr[2]) > tolerance) {
                        return String("Field '") + path + String("' mismatch");
                    }
                }
            } else if (type_hint == "Color") {
                if (actual.get_type() != Variant::COLOR) {
                    return String("Field '") + path + String("' is not Color");
                }
                const Color actual_c = actual;
                const Array expected_arr = expected_val;
                const double tolerance = check.get("tolerance", 0.0001);
                if (expected_arr.size() >= 3) {
                    const double er = expected_arr[0];
                    const double eg = expected_arr[1];
                    const double eb = expected_arr[2];
                    const double ea = expected_arr.size() > 3 ? (double)expected_arr[3] : 1.0;
                    if (Math::abs(actual_c.r - er) > tolerance ||
                        Math::abs(actual_c.g - eg) > tolerance ||
                        Math::abs(actual_c.b - eb) > tolerance ||
                        Math::abs(actual_c.a - ea) > tolerance) {
                        return String("Field '") + path + String("' color mismatch");
                    }
                }
            } else if (type_hint == "float" || type_hint == "double") {
                const double tolerance = check.get("tolerance", 0.0001);
                const double expected_num = expected_val;
                const double actual_num = actual;
                if (Math::abs(actual_num - expected_num) > tolerance) {
                    return String("Field '") + path + String("' float mismatch: expected ") +
                           String::num(expected_num) + String(", got ") + String::num(actual_num);
                }
            } else if (type_hint == "int" || type_hint == "integer") {
                const int64_t expected_int = expected_val;
                const int64_t actual_int = actual;
                if (actual_int != expected_int) {
                    return String("Field '") + path + String("' int mismatch: expected ") +
                           String::num_int64(expected_int) + String(", got ") + String::num_int64(actual_int);
                }
            } else if (type_hint == "bool") {
                if ((bool)actual != (bool)expected_val) {
                    return String("Field '") + path + String("' bool mismatch");
                }
            } else if (type_hint == "String" || type_hint == "string") {
                const String actual_s = actual;
                const String expected_s = expected_val;
                if (actual_s != expected_s) {
                    return String("Field '") + path + String("' string mismatch: expected '") +
                           expected_s + String("', got '") + actual_s + String("'");
                }
            } else {
                // Generic equality
                if (actual != expected_val) {
                    return String("Field '") + path + String("' mismatch: expected ") +
                           JSON::stringify(expected_val) + String(", got ") + JSON::stringify(actual);
                }
            }
        }
    }

    // 4. error_contains: if error, check substring
    if (expect.has("error_contains")) {
        if (!result.has("error")) {
            return String("Expected error containing '") + expect["error"].operator String() + String("', but no error");
        }
        const String error_msg = result["error"];
        const String needle = expect["error_contains"];
        if (error_msg.find(needle) == -1) {
            return String("Error message doesn't contain '") + needle + String("': ") + error_msg;
        }
    }

    return String(); // pass
}

} // namespace godot_mcp
