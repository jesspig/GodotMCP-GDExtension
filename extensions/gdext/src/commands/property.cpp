// commands/property.cpp — 21 property get/set tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary cmd_get_node_position(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Vector2 v = n->get("position");
    Dictionary r; r["node_path"] = p; r["x"] = v.x; r["y"] = v.y; return r;
}
Dictionary cmd_set_node_position(const Dictionary &a) {
    String p = args_string(a, "node_path");
    double x = args_float(a, "x", 0), y = args_float(a, "y", 0);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "position", Vector2(x, y), "Set position for " + p);
    Vector2 v = n->get("position");
    Dictionary r; r["node_path"] = p; r["property"] = "position"; r["x"] = v.x; r["y"] = v.y; return r;
}
Dictionary cmd_get_node_rotation(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    double deg = (double)n->get("rotation_degrees");
    Dictionary r; r["node_path"] = p; r["degrees"] = deg; return r;
}
Dictionary cmd_set_node_rotation(const Dictionary &a) {
    String p = args_string(a, "node_path");
    double deg = args_float(a, "degrees", 0);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "rotation_degrees", deg, "Set rotation for " + p);
    double actual = (double)n->get("rotation_degrees");
    Dictionary r; r["node_path"] = p; r["property"] = "rotation_degrees"; r["degrees"] = actual; return r;
}
Dictionary cmd_get_node_scale(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Vector2 v = n->get("scale");
    Dictionary r; r["node_path"] = p; r["x"] = v.x; r["y"] = v.y; return r;
}
Dictionary cmd_set_node_scale(const Dictionary &a) {
    String p = args_string(a, "node_path");
    double x = args_float(a, "x", 1), y = args_float(a, "y", 1);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "scale", Vector2(x, y), "Set scale for " + p);
    Vector2 v = n->get("scale");
    Dictionary r; r["node_path"] = p; r["property"] = "scale"; r["x"] = v.x; r["y"] = v.y; return r;
}
Dictionary cmd_get_node_visible(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    bool vis = n->get("visible");
    Dictionary r; r["node_path"] = p; r["visible"] = vis; return r;
}
Dictionary cmd_set_node_visible(const Dictionary &a) {
    String p = args_string(a, "node_path");
    bool v = args_bool(a, "visible", true);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "visible", v, "Set visible for " + p);
    Dictionary r; r["node_path"] = p; r["property"] = "visible"; r["visible"] = (bool)n->get("visible"); return r;
}
Dictionary cmd_get_node_modulate(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Color c = n->get("modulate");
    Dictionary r; r["node_path"] = p; r["r"] = c.r; r["g"] = c.g; r["b"] = c.b; r["a"] = c.a; return r;
}
Dictionary cmd_set_node_modulate(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Color c(args_float(a, "r", 1), args_float(a, "g", 1), args_float(a, "b", 1), args_float(a, "a", 1));
    String color_str = args_string(a, "color");
    if (!color_str.is_empty()) { c = Color::html(color_str); }
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "modulate", c, "Set modulate for " + p);
    Color actual = n->get("modulate");
    Dictionary r; r["node_path"] = p; r["property"] = "modulate"; r["r"] = actual.r; r["g"] = actual.g; r["b"] = actual.b; r["a"] = actual.a; return r;
}
Dictionary cmd_get_node_z_index(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    int64_t z = n->get("z_index");
    Dictionary r; r["node_path"] = p; r["z_index"] = z; return r;
}
Dictionary cmd_set_node_z_index(const Dictionary &a) {
    String p = args_string(a, "node_path");
    int64_t z = args_int(a, "z_index", 0);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "z_index", z, "Set z_index for " + p);
    int64_t actual = n->get("z_index");
    Dictionary r; r["node_path"] = p; r["property"] = "z_index"; r["z_index"] = actual; return r;
}
Dictionary cmd_get_node_text(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    String t = n->get("text");
    Dictionary r; r["node_path"] = p; r["text"] = t; return r;
}
Dictionary cmd_set_node_text(const Dictionary &a) {
    String p = args_string(a, "node_path");
    String t = args_string(a, "text");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    undoable_set(n, "text", t, "Set text for " + p);
    Dictionary r; r["node_path"] = p; r["property"] = "text"; r["text"] = (String)n->get("text"); return r;
}
Dictionary check_collision_compat(Node *n, const String &p) {
    String cls = n->get_class();
    if (cls == "CollisionObject2D" || cls == "CollisionObject3D" ||
        cls.begins_with("CharacterBody2D") || cls.begins_with("CharacterBody3D") ||
        cls.begins_with("Area2D") || cls.begins_with("Area3D") ||
        cls.begins_with("StaticBody") || cls.begins_with("RigidBody") ||
        cls.begins_with("AnimatableBody"))
        return Dictionary();
    return make_error("Node '" + p + "' (" + cls + ") does not have collision_layer/mask");
}
Dictionary cmd_get_node_collision_layer(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary err = check_collision_compat(n, p); if (err.has("error")) return err;
    int64_t v = n->get("collision_layer");
    Dictionary r; r["node_path"] = p; r["collision_layer"] = v; return r;
}
Dictionary cmd_set_node_collision_layer(const Dictionary &a) {
    String p = args_string(a, "node_path");
    int64_t layer = args_int(a, "layer", 1);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary err = check_collision_compat(n, p); if (err.has("error")) return err;
    undoable_set(n, "collision_layer", layer, "Set collision_layer for " + p);
    int64_t actual = n->get("collision_layer");
    Dictionary r; r["node_path"] = p; r["property"] = "collision_layer"; r["layer"] = actual; return r;
}
Dictionary cmd_get_node_collision_mask(const Dictionary &a) {
    String p = args_string(a, "node_path");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary err = check_collision_compat(n, p); if (err.has("error")) return err;
    int64_t v = n->get("collision_mask");
    Dictionary r; r["node_path"] = p; r["collision_mask"] = v; return r;
}
Dictionary cmd_set_node_collision_mask(const Dictionary &a) {
    String p = args_string(a, "node_path");
    int64_t mask = args_int(a, "mask", 1);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Dictionary err = check_collision_compat(n, p); if (err.has("error")) return err;
    undoable_set(n, "collision_mask", mask, "Set collision_mask for " + p);
    int64_t actual = n->get("collision_mask");
    Dictionary r; r["node_path"] = p; r["property"] = "collision_mask"; r["mask"] = actual; return r;
}
Dictionary cmd_set_node_unique_name(const Dictionary &a) {
    String p = args_string(a, "node_path");
    bool unique = args_bool(a, "unique", true);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    EditorUndoRedoManager *ur = get_undo_redo(); if (!ur) { n->set("unique_name_in_owner", unique); return Dictionary(); }
    bool old = n->is_unique_name_in_owner();
    n->set("unique_name_in_owner", unique);
    ur->create_action("Set unique_name for " + p);
    ur->add_do_method(n, "set_unique_name_in_owner", Array::make(unique));
    ur->add_undo_method(n, "set_unique_name_in_owner", Array::make(old));
    ur->commit_action(false);
    Dictionary r; r["node_path"] = p; r["unique_name_in_owner"] = unique; return r;
}
Dictionary cmd_get_node_texture(const Dictionary &a) {
    String p = args_string(a, "node_path");
    String prop = args_string(a, "property", "texture");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Variant val = n->get(prop);
    if (val.get_type() == Variant::NIL) return make_error("Node '" + p + "' does not have property '" + prop + "'");
    Ref<Texture2D> tex = val;
    Dictionary r; r["node_path"] = p; r["property"] = prop;
    if (tex.is_valid()) r["texture_path"] = tex->get_path(); else r["texture_path"] = Variant();
    return r;
}
Dictionary cmd_set_node_texture(const Dictionary &a) {
    String p = args_string(a, "node_path");
    String prop = args_string(a, "property", "texture");
    String tex_path = args_string(a, "texture_path");
    if (tex_path.is_empty()) return make_error("missing 'texture_path'");
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *n = resolve_node(root, p); if (!n) return make_error("node not found: " + p);
    Ref<Texture2D> tex = ResourceLoader::get_singleton()->load(tex_path);
    if (tex.is_null()) return make_error("Failed to load Texture2D from '" + tex_path + "'");
    Variant old = n->get(prop);
    n->set(prop, tex);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) { ur->create_action("Set " + prop + " for " + p); ur->add_do_property(n, prop, tex); ur->add_undo_property(n, prop, old); ur->commit_action(false); }
    Dictionary r; r["node_path"] = p; r["property"] = prop; r["texture_path"] = tex_path; return r;
}
} // namespace

void register_property(HandlerRegistry &reg) {
    reg.register_tool("get_node_position", cmd_get_node_position);
    reg.register_tool("set_node_position", cmd_set_node_position);
    reg.register_tool("get_node_rotation", cmd_get_node_rotation);
    reg.register_tool("set_node_rotation", cmd_set_node_rotation);
    reg.register_tool("get_node_scale", cmd_get_node_scale);
    reg.register_tool("set_node_scale", cmd_set_node_scale);
    reg.register_tool("get_node_visible", cmd_get_node_visible);
    reg.register_tool("set_node_visible", cmd_set_node_visible);
    reg.register_tool("get_node_modulate", cmd_get_node_modulate);
    reg.register_tool("set_node_modulate", cmd_set_node_modulate);
    reg.register_tool("get_node_z_index", cmd_get_node_z_index);
    reg.register_tool("set_node_z_index", cmd_set_node_z_index);
    reg.register_tool("get_node_text", cmd_get_node_text);
    reg.register_tool("set_node_text", cmd_set_node_text);
    reg.register_tool("get_node_collision_layer", cmd_get_node_collision_layer);
    reg.register_tool("set_node_collision_layer", cmd_set_node_collision_layer);
    reg.register_tool("get_node_collision_mask", cmd_get_node_collision_mask);
    reg.register_tool("set_node_collision_mask", cmd_set_node_collision_mask);
    reg.register_tool("set_node_unique_name", cmd_set_node_unique_name);
    reg.register_tool("get_node_texture", cmd_get_node_texture);
    reg.register_tool("set_node_texture", cmd_set_node_texture);
}
} // namespace godot_mcp