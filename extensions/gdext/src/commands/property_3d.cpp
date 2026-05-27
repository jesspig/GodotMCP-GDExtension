// commands/property_3d.cpp — 6 Node3D property get/set tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/variant/vector3.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary check_node3d(Node *n, const String &p) {
    if (n->is_class("Node3D")) return Dictionary();
    return make_error("Node '" + p + "' (" + n->get_class() + ") is not a Node3D");
}

Dictionary cmd_get_position_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary e = check_node3d(n, p); if (e.has("error")) return e;
    Vector3 v = n->get("position");
    Dictionary r; r["node_path"] = p; r["x"] = v.x; r["y"] = v.y; r["z"] = v.z; return r;
}
Dictionary cmd_set_position_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    double x = args_float(a, "x", 0), y = args_float(a, "y", 0), z = args_float(a, "z", 0);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary e = check_node3d(n, p); if (e.has("error")) return e;
    undoable_set(n, "position", Vector3(x, y, z), "Set 3D position for " + p);
    Vector3 actual = n->get("position");
    Dictionary r; r["node_path"] = p; r["property"] = "position"; r["x"] = actual.x; r["y"] = actual.y; r["z"] = actual.z; return r;
}
Dictionary cmd_get_rotation_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary e = check_node3d(n, p); if (e.has("error")) return e;
    Vector3 v = n->get("rotation_degrees");
    Dictionary r; r["node_path"] = p; r["x"] = v.x; r["y"] = v.y; r["z"] = v.z; return r;
}
Dictionary cmd_set_rotation_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    double x = args_float(a, "rot_x", args_float(a, "x", 0));
    double y = args_float(a, "rot_y", args_float(a, "y", 0));
    double z = args_float(a, "rot_z", args_float(a, "z", 0));
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary e = check_node3d(n, p); if (e.has("error")) return e;
    undoable_set(n, "rotation_degrees", Vector3(x, y, z), "Set 3D rotation for " + p);
    Vector3 actual = n->get("rotation_degrees");
    Dictionary r; r["node_path"] = p; r["property"] = "rotation_degrees"; r["x"] = actual.x; r["y"] = actual.y; r["z"] = actual.z; return r;
}
Dictionary cmd_get_scale_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary e = check_node3d(n, p); if (e.has("error")) return e;
    Vector3 v = n->get("scale");
    Dictionary r; r["node_path"] = p; r["x"] = v.x; r["y"] = v.y; r["z"] = v.z; return r;
}
Dictionary cmd_set_scale_3d(const Dictionary &a) {
    String p = args_string(a, "node_path");
    double x = args_float(a, "scale_x", args_float(a, "x", 1));
    double y = args_float(a, "scale_y", args_float(a, "y", 1));
    double z = args_float(a, "scale_z", args_float(a, "z", 1));
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary e = check_node3d(n, p); if (e.has("error")) return e;
    undoable_set(n, "scale", Vector3(x, y, z), "Set 3D scale for " + p);
    Vector3 actual = n->get("scale");
    Dictionary r; r["node_path"] = p; r["property"] = "scale"; r["x"] = actual.x; r["y"] = actual.y; r["z"] = actual.z; return r;
}
} // namespace

void register_property_3d(HandlerRegistry &reg) {
    reg.register_tool("get_node_position_3d", cmd_get_position_3d);
    reg.register_tool("set_node_position_3d", cmd_set_position_3d);
    reg.register_tool("get_node_rotation_3d", cmd_get_rotation_3d);
    reg.register_tool("set_node_rotation_3d", cmd_set_rotation_3d);
    reg.register_tool("get_node_scale_3d", cmd_get_scale_3d);
    reg.register_tool("set_node_scale_3d", cmd_set_scale_3d);
}
} // namespace godot_mcp