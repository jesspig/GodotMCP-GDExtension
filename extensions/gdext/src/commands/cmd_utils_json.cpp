// =====================================================================
// commands/cmd_utils_json.cpp — JSON <-> Variant conversion helpers.
//
// Split out from cmd_utils.cpp purely for file-size readability. These
// functions translate between Godot's structured Variant types and the
// plain JSON shape the MCP protocol uses on the wire.
//
// Naming convention (mirrors Rust):
//   variant_to_json(v)   == Rust v2j(&v)   — Variant -> JSON-friendly
//   json_to_variant(jv)  == Rust j2v(&v)   — JSON Dict/Array -> Variant
// =====================================================================

#include "cmd_utils.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

using namespace godot;

namespace godot_mcp {

// ---------------------------------------------------------------------
// variant_to_json
// ---------------------------------------------------------------------

Variant variant_to_json(const Variant &v) {
    switch (v.get_type()) {
        case Variant::NIL:
        case Variant::BOOL:
        case Variant::INT:
        case Variant::FLOAT:
        case Variant::STRING:
        case Variant::STRING_NAME:
            // Primitives pass through unchanged; JSON::stringify handles them.
            if (v.get_type() == Variant::STRING_NAME) {
                return String(v);
            }
            return v;

        case Variant::VECTOR2: {
            const Vector2 vec = v;
            Dictionary d;
            d["x"] = vec.x;
            d["y"] = vec.y;
            return d;
        }
        case Variant::VECTOR2I: {
            const Vector2i vec = v;
            Dictionary d;
            d["x"] = vec.x;
            d["y"] = vec.y;
            return d;
        }
        case Variant::VECTOR3: {
            const Vector3 vec = v;
            Dictionary d;
            d["x"] = vec.x;
            d["y"] = vec.y;
            d["z"] = vec.z;
            return d;
        }
        case Variant::VECTOR3I: {
            const Vector3i vec = v;
            Dictionary d;
            d["x"] = vec.x;
            d["y"] = vec.y;
            d["z"] = vec.z;
            return d;
        }
        case Variant::VECTOR4: {
            const Vector4 vec = v;
            Dictionary d;
            d["x"] = vec.x;
            d["y"] = vec.y;
            d["z"] = vec.z;
            d["w"] = vec.w;
            return d;
        }
        case Variant::QUATERNION: {
            const Quaternion q = v;
            Dictionary d;
            d["x"] = q.x;
            d["y"] = q.y;
            d["z"] = q.z;
            d["w"] = q.w;
            return d;
        }
        case Variant::COLOR: {
            const Color c = v;
            Dictionary d;
            d["r"] = c.r;
            d["g"] = c.g;
            d["b"] = c.b;
            d["a"] = c.a;
            return d;
        }
        case Variant::RECT2: {
            const Rect2 r = v;
            Dictionary d;
            d["position"] = variant_to_json(r.position);
            d["size"] = variant_to_json(r.size);
            return d;
        }
        case Variant::DICTIONARY: {
            const Dictionary src = v;
            Dictionary dst;
            const Array keys = src.keys();
            for (int i = 0; i < keys.size(); ++i) {
                const Variant key = keys[i];
                // JSON object keys must be strings.
                const String key_str = key.stringify();
                dst[key_str] = variant_to_json(src[key]);
            }
            return dst;
        }
        case Variant::ARRAY: {
            const Array src = v;
            Array dst;
            dst.resize(src.size());
            for (int i = 0; i < src.size(); ++i) {
                dst[i] = variant_to_json(src[i]);
            }
            return dst;
        }
        case Variant::PACKED_STRING_ARRAY: {
            const PackedStringArray src = v;
            Array dst;
            dst.resize(src.size());
            for (int i = 0; i < src.size(); ++i) {
                dst[i] = src[i];
            }
            return dst;
        }
        case Variant::OBJECT: {
            // Encode Resources by their res:// path so the other side can
            // round-trip the value via json_to_variant.
            Object *obj = (Object *)v;
            if (!obj) {
                return Variant();
            }
            Resource *res = Object::cast_to<Resource>(obj);
            if (res) {
                Dictionary d;
                d["resource_path"] = res->get_path();
                return d;
            }
            return obj->to_string();
        }
        default:
            return v.stringify();
    }
}

// ---------------------------------------------------------------------
// json_to_variant
// ---------------------------------------------------------------------

namespace {

// Heuristic: look at a Dictionary's keys and return the most specific
// Godot type it represents, or NIL when it should stay a Dictionary.
Variant dict_to_specific_type(const Dictionary &d) {
    const bool has_x = d.has("x");
    const bool has_y = d.has("y");
    const bool has_z = d.has("z");
    const bool has_w = d.has("w");

    if (has_x && has_y && !has_z && !has_w) {
        return Vector2((double)d["x"], (double)d["y"]);
    }
    if (has_x && has_y && has_z && !has_w) {
        return Vector3((double)d["x"], (double)d["y"], (double)d["z"]);
    }
    if (has_x && has_y && has_z && has_w) {
        // Prefer Vector4 — call sites that need Quaternion can convert.
        return Vector4((double)d["x"], (double)d["y"], (double)d["z"], (double)d["w"]);
    }
    if (d.has("r") && d.has("g") && d.has("b")) {
        const double a = d.has("a") ? (double)d["a"] : 1.0;
        return Color((double)d["r"], (double)d["g"], (double)d["b"], a);
    }
    if (d.has("position") && d.has("size")) {
        const Variant pos_v = d["position"];
        const Variant size_v = d["size"];
        // Recurse so the position/size become Vector2 first.
        const Variant pos = pos_v.get_type() == Variant::DICTIONARY
                                ? dict_to_specific_type(pos_v)
                                : pos_v;
        const Variant size = size_v.get_type() == Variant::DICTIONARY
                                 ? dict_to_specific_type(size_v)
                                 : size_v;
        if (pos.get_type() == Variant::VECTOR2 && size.get_type() == Variant::VECTOR2) {
            return Rect2((Vector2)pos, (Vector2)size);
        }
    }
    if (d.has("resource_path")) {
        const String path = d["resource_path"];
        if (!path.is_empty()) {
            return ResourceLoader::get_singleton()->load(path);
        }
    }
    return Variant();  // NIL -> caller keeps the original Dictionary.
}

}  // namespace

Variant json_to_variant(const Variant &jv) {
    switch (jv.get_type()) {
        case Variant::STRING: {
            const String s = jv;
            if (s.begins_with("res://") || s.begins_with("user://")) {
                Ref<Resource> res = ResourceLoader::get_singleton()->load(s);
                if (res.is_valid()) {
                    return res;
                }
            }
            return jv;
        }
        case Variant::DICTIONARY: {
            const Dictionary d = jv;
            const Variant specific = dict_to_specific_type(d);
            if (specific.get_type() != Variant::NIL) {
                return specific;
            }
            // Recurse into values to handle nested specific types.
            Dictionary out;
            const Array keys = d.keys();
            for (int i = 0; i < keys.size(); ++i) {
                const Variant k = keys[i];
                out[k] = json_to_variant(d[k]);
            }
            return out;
        }
        case Variant::ARRAY: {
            const Array src = jv;
            // 2/3/4-element numeric arrays are interpreted as Vector2/3 or Color.
            bool all_numeric = src.size() >= 2 && src.size() <= 4;
            for (int i = 0; i < src.size() && all_numeric; ++i) {
                const Variant::Type t = src[i].get_type();
                if (t != Variant::INT && t != Variant::FLOAT) {
                    all_numeric = false;
                }
            }
            if (all_numeric) {
                if (src.size() == 2) {
                    return Vector2((double)src[0], (double)src[1]);
                }
                if (src.size() == 3) {
                    return Vector3((double)src[0], (double)src[1], (double)src[2]);
                }
                if (src.size() == 4) {
                    return Color((double)src[0], (double)src[1], (double)src[2], (double)src[3]);
                }
            }
            Array out;
            out.resize(src.size());
            for (int i = 0; i < src.size(); ++i) {
                out[i] = json_to_variant(src[i]);
            }
            return out;
        }
        default:
            return jv;
    }
}

}  // namespace godot_mcp
