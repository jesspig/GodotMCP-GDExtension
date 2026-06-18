#pragma once

#include "type_utils.hpp"
#include "yaml_parser.hpp"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot_mcp {

// Verify a single field value against an expected value with type-aware comparison.
inline godot::String verify_field(const godot::String &context,
                                   const godot::Variant &actual,
                                   const godot::Variant &expected_val,
                                   const godot::String &type_hint,
                                   double tolerance) {
    const godot::String err = compare_variant_fields(actual, expected_val, type_hint, tolerance);
    if (err.is_empty()) return godot::String();
    return context + godot::String(" ") + err;
}

// Load a .tscn file and verify its node tree structure and properties.
inline godot::Array verify_scene_file(const godot::String &scene_path,
                                       const godot::Array &node_specs) {
    using namespace godot;
    Array errors;

    Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(scene_path);
    if (scene.is_null()) {
        errors.push_back(String("Cannot load scene: ") + scene_path);
        return errors;
    }

    Node *instance = scene->instantiate();
    if (!instance) {
        errors.push_back(String("Cannot instantiate scene: ") + scene_path);
        return errors;
    }
    struct NodeGuard { Node *n; ~NodeGuard() { if (n) memdelete(n); } } node_guard{instance};

    for (int i = 0; i < node_specs.size(); ++i) {
        const Dictionary spec = node_specs[i];
        const String node_path = spec.get("path", "");
        const String expected_type = spec.get("type", "");
        const bool exists = spec.get("exists", true);

        Node *found = instance->get_node<Node>(NodePath(node_path));
        if (exists && !found) {
            errors.push_back(String("Node '") + node_path + String("' not found in ") + scene_path);
            continue;
        }
        if (!exists && found) {
            errors.push_back(String("Node '") + node_path + String("' exists but should not in ") + scene_path);
            continue;
        }
        if (!found) continue;

        // Type check
        if (!expected_type.is_empty()) {
            const String actual_type = String(found->get_class());
            if (actual_type != expected_type) {
                errors.push_back(String("Node '") + node_path + String("' type mismatch: expected ") +
                                 expected_type + String(", got ") + actual_type);
            }
        }

        // Property checks
        if (spec.has("properties")) {
            const Array props = spec["properties"];
            for (int p = 0; p < props.size(); ++p) {
                const Dictionary prop = props[p];
                const String prop_path = prop.get("path", "");
                const Variant expected_val = prop.get("expect", Variant());
                const String type_hint = prop.get("type", "");
                const double tolerance = prop.get("tolerance", kDefaultTolerance);

                // Navigate nested properties (e.g. "position.x")
                Variant actual_val;
                if (prop_path.find(".") >= 0) {
                    const Array parts = prop_path.split(".");
                    Variant curr = Variant(found);
                    for (int pp = 0; pp < parts.size(); ++pp) {
                        if (curr.get_type() == Variant::DICTIONARY) {
                            Dictionary d = curr;
                            curr = d.get(parts[pp], Variant());
                        } else if (curr.get_type() == Variant::OBJECT) {
                            Object *obj = Object::cast_to<Object>(curr.operator Object*());
                            if (obj) {
                                curr = obj->get(StringName(String(parts[pp])));
                            } else {
                                curr = Variant();
                            }
                        } else {
                            curr = Variant();
                        }
                    }
                    actual_val = curr;
                } else {
                    actual_val = found->get(StringName(prop_path));
                }

                const String err = verify_field(
                    String("'") + scene_path + String("' node '") + node_path +
                    String("' property '") + prop_path + String("'"),
                    actual_val, expected_val, type_hint, tolerance);
                if (!err.is_empty()) {
                    errors.push_back(err);
                }
            }
        }
    }

    return errors;
}

// Verify raw text file content for substring presence/absence.
inline godot::Array verify_raw_text(const godot::String &file_path,
                                     const godot::Dictionary &checks) {
    using namespace godot;
    Array errors;

    Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
    if (f.is_null()) {
        errors.push_back(String("Cannot open file: ") + file_path);
        return errors;
    }
    const String content = f->get_as_text();
    f->close();

    if (checks.has("contains")) {
        const String needle = checks["contains"];
        if (content.find(needle) == -1) {
            errors.push_back(String("File '") + file_path + String("' does not contain expected string"));
        }
    }
    if (checks.has("not_contains")) {
        const String needle = checks["not_contains"];
        if (content.find(needle) != -1) {
            errors.push_back(String("File '") + file_path + String("' contains unexpected string"));
        }
    }

    return errors;
}

// Verify project.godot ConfigFile settings.
inline godot::Array verify_project_godot(const godot::Array &setting_checks) {
    using namespace godot;
    Array errors;

    Ref<ConfigFile> config;
    config.instantiate();
    const Error err = config->load("res://project.godot");
    if (err != OK) {
        errors.push_back("Cannot load project.godot");
        return errors;
    }

    for (int i = 0; i < setting_checks.size(); ++i) {
        const Dictionary check = setting_checks[i];
        const String section = check.get("section", "");
        const String key = check.get("key", "");
        const Variant expected_val = check.get("expect", Variant());
        const double tolerance = check.get("tolerance", kDefaultTolerance);

        if (!config->has_section_key(section, key)) {
            errors.push_back(String("project.godot missing [") + section + String("] ") + key);
            continue;
        }

        const Variant actual = config->get_value(section, key, Variant());
        const String err_msg = verify_field(
            String("project.godot [") + section + String("] ") + key,
            actual, expected_val,
            check.get("type", ""),
            tolerance);
        if (!err_msg.is_empty()) {
            errors.push_back(err_msg);
        }
    }

    return errors;
}

// Top-level disk verification dispatcher.
//   disk_verify: the "disk_verify" block from YAML config
// Returns Array of error strings (empty = all pass).
inline godot::Array run_disk_verification(const godot::Dictionary &disk_verify) {
    using namespace godot;
    Array all_errors;

    if (disk_verify.has("scene")) {
        const Dictionary scene_verify = disk_verify["scene"];
        const String scene_path = scene_verify.get("path", "");
        const Array nodes = scene_verify.get("nodes", Array());
        const Array errs = verify_scene_file(scene_path, nodes);
        for (int i = 0; i < errs.size(); ++i) all_errors.push_back(errs[i]);
    }

    if (disk_verify.has("project_settings")) {
        const Array settings = disk_verify["project_settings"];
        const Array errs = verify_project_godot(settings);
        for (int i = 0; i < errs.size(); ++i) all_errors.push_back(errs[i]);
    }

    if (disk_verify.has("raw_text")) {
        const Array raw_checks = disk_verify["raw_text"];
        for (int i = 0; i < raw_checks.size(); ++i) {
            const Dictionary check = raw_checks[i];
            const String file_path = check.get("path", "");
            const Array errs = verify_raw_text(file_path, check);
            for (int j = 0; j < errs.size(); ++j) all_errors.push_back(errs[j]);
        }
    }

    return all_errors;
}

} // namespace godot_mcp
