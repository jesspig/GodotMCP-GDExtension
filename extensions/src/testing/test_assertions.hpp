#pragma once

#include "type_utils.hpp"
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
inline godot::Dictionary unwrap_result(const godot::Dictionary &result) {
    using namespace godot;
    if (result.has("success") && result.has("data")) {
        const Variant data = result["data"];
        if (data.get_type() == Variant::DICTIONARY) {
            return data;
        }
    }
    return result;
}

inline godot::String run_assertions(const godot::Dictionary &expect,
                                    const godot::Dictionary &result) {
    using namespace godot;

    const Dictionary data = unwrap_result(result);

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
            if (!data.has(key)) {
                return String("Missing expected key: ") + key;
            }
        }
    }

    // 3. field_checks: array of {path or key, expect or value, type, tolerance?, not_empty?}
    if (expect.has("field_checks")) {
        const Array checks = expect["field_checks"];
        for (int i = 0; i < checks.size(); ++i) {
            const Dictionary check = checks[i];
            const String path = check.has("path") ? String(check["path"]) : String(check.get("key", ""));
            const Variant expected_val = check.has("expect") ? check["expect"] : check.get("value", Variant());
            String type_hint = normalize_type_hint(check.get("type", ""));
            const bool not_empty = check.get("not_empty", false);

            Variant actual = data;
            if (!path.is_empty()) {
                String remaining = path;
                while (!remaining.is_empty()) {
                    if (actual.get_type() != Variant::DICTIONARY) {
                        return String("Field path '") + path + String("' cannot be traversed at '") + remaining + String("'");
                    }
                    Dictionary d = actual;
                    int dot = static_cast<int>(remaining.find("."));
                    if (dot != -1) {
                        String key = remaining.substr(0, dot);
                        remaining = remaining.substr(dot + 1);
                        actual = d.get(key, Variant());
                    } else {
                        actual = d.get(remaining, Variant());
                        break;
                    }
                }
            }

            // not_empty check
            if (not_empty) {
                if (actual.get_type() == Variant::NIL) {
                    return String("Field '") + path + String("' is nil, expected non-empty");
                }
                if (actual.get_type() == Variant::STRING && (static_cast<String>(actual)).is_empty()) {
                    return String("Field '") + path + String("' is empty string");
                }
                if (actual.get_type() == Variant::ARRAY && (static_cast<Array>(actual)).is_empty()) {
                    return String("Field '") + path + String("' is empty array");
                }
                if (actual.get_type() == Variant::DICTIONARY && (static_cast<Dictionary>(actual)).is_empty()) {
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

            const double tolerance = check.get("tolerance", kDefaultTolerance);
            const String cmp_err = compare_variant_fields(actual, expected_val, type_hint, tolerance);
            if (!cmp_err.is_empty()) {
                return String("Field '") + path + String("' ") + cmp_err;
            }
        }
    }

    // 4. error_contains: if error, check substring
    if (expect.has("error_contains")) {
        if (!result.has("error")) {
            return String("Expected error containing '") + expect["error_contains"].operator String() + String("', but no error");
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
