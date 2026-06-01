#pragma once

#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

inline String variant_type_name(int64_t tid) {
    switch (tid) {
        case 0: return "NIL"; case 1: return "bool"; case 2: return "int"; case 3: return "float";
        case 4: return "String"; case 5: return "Vector2"; case 6: return "Vector2i";
        case 7: return "Rect2"; case 8: return "Rect2i"; case 9: return "Vector3";
        case 10: return "Vector3i"; case 11: return "Transform2D"; case 12: return "Vector4";
        case 13: return "Vector4i"; case 14: return "Plane"; case 15: return "Quaternion";
        case 16: return "AABB"; case 17: return "Basis"; case 18: return "Transform3D";
        case 19: return "Projection"; case 20: return "Color"; case 21: return "StringName";
        case 22: return "NodePath"; case 23: return "RID"; case 24: return "Object";
        case 25: return "Dictionary"; case 26: return "Array";
        case 27: return "PackedByteArray"; case 28: return "PackedInt32Array";
        case 29: return "PackedInt64Array"; case 30: return "PackedFloat32Array";
        case 31: return "PackedFloat64Array"; case 32: return "PackedStringArray";
        case 33: return "PackedVector2Array"; case 34: return "PackedVector3Array";
        case 35: return "PackedColorArray"; default: return "Unknown";
    }
}

inline Dictionary serialize_tree(Node *n, Node *root, int64_t depth, int64_t max) {
    Dictionary r;
    r["name"] = n->get_name();
    r["type"] = n->get_class();
    r["path"] = relative_path(root, n);
    r["visible"] = (bool)n->get("visible");
    Ref<Script> sc = n->get("script");
    if (sc.is_valid()) r["script"] = sc->get_path(); else r["script"] = Variant();
    Array children;
    if (depth < max) {
        for (int64_t i = 0; i < n->get_child_count(); i++) {
            Node *c = Object::cast_to<Node>(n->get_child(i));
            if (c) children.append(serialize_tree(c, root, depth + 1, max));
        }
    }
    r["children"] = children;
    r["child_count"] = (int64_t)children.size();
    return r;
}

inline Dictionary dump_prop_list(Node *n) {
    Array pl = n->get_property_list();
    Array out;
    for (int i = 0; i < pl.size(); i++) {
        Dictionary d = pl[i];
        int64_t usage = (int64_t)d.get("usage", 0);
        if (usage & 0x00000040) continue;
        Dictionary e;
        e["name"] = d.get("name", "");
        e["type"] = variant_type_name((int64_t)d.get("type", 0));
        e["hint"] = d.get("hint", 0);
        e["hint_string"] = d.get("hint_string", "");
        out.append(e);
    }
    Dictionary r;
    r["properties"] = out;
    return r;
}

} // namespace godot_mcp
