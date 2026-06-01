// commands/collision.cpp â€?add_circle/rectangle_collision

#include "cmd_utils.hpp"
#include "server/registry/handler_registry.hpp"
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary do_collision_shape(const Dictionary &a, bool circle) {
    String p = args_string(a, "node_path");
    double radius = args_float(a, "radius", 10);
    double w = args_float(a, "width", 20), h = args_float(a, "height", 100);
    Node *root = get_root(); if (!root) return make_error("no scene");
    Node *target = resolve_node(root, p); if (!target) return make_error("node not found: " + p);

    Variant shape;
    if (circle) {
        Ref<CircleShape2D> cs; cs.instantiate(); cs->set_radius(radius); shape = cs;
    } else {
        Ref<RectangleShape2D> rs; rs.instantiate(); rs->set_size(Vector2(w, h)); shape = rs;
    }

    // If target IS a CollisionShape2D, set its shape directly
    if (target->get_class() == "CollisionShape2D") {
        Variant old = target->get("shape");
        target->set("shape", shape);
        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) { ur->create_action("Set collision shape"); ur->add_do_property(target, "shape", shape); ur->add_undo_property(target, "shape", old); ur->commit_action(false); }
        Dictionary r; r["node_path"] = p; r["shape"] = circle ? "CircleShape2D" : "RectangleShape2D"; r["mode"] = "set_on_existing"; return r;
    }
    // Check existing CollisionShape2D child
    for (int64_t i = 0; i < target->get_child_count(); i++) {
        Node *child = Object::cast_to<Node>(target->get_child(i));
        if (child && child->get_class() == "CollisionShape2D") {
            Variant old = child->get("shape");
            child->set("shape", shape);
            EditorUndoRedoManager *ur = get_undo_redo();
            if (ur) { ur->create_action("Set collision shape"); ur->add_do_property(child, "shape", shape); ur->add_undo_property(child, "shape", old); ur->commit_action(false); }
            Dictionary r; r["node_path"] = p; r["shape"] = circle ? "CircleShape2D" : "RectangleShape2D"; r["mode"] = "set_on_existing"; return r;
        }
    }
    // Create new CollisionShape2D child
    Node *shape_node = Object::cast_to<Node>(ClassDB::instantiate("CollisionShape2D"));
    if (!shape_node) return make_error("Failed to instantiate CollisionShape2D");
    shape_node->set_name("CollisionShape2D");
    shape_node->set("shape", shape);
    EditorUndoRedoManager *ur = get_undo_redo();
    if (ur) {
        ur->create_action("Add collision shape");
        ur->add_do_method(target, "add_child", Variant(shape_node));
        ur->add_do_method(shape_node, "set_owner", Variant(root));
        ur->add_do_reference(shape_node);
        ur->add_undo_method(target, "remove_child", Variant(shape_node));
        ur->commit_action();
    }
    Dictionary r; r["node_path"] = p; r["shape"] = circle ? "CircleShape2D" : "RectangleShape2D"; r["mode"] = "created_child"; return r;
}

Dictionary cmd_add_circle_collision(const Dictionary &a) { return do_collision_shape(a, true); }
Dictionary cmd_add_rectangle_collision(const Dictionary &a) { return do_collision_shape(a, false); }
} // namespace

void register_collision(HandlerRegistry &reg) {
    reg.register_tool("add_circle_collision", cmd_add_circle_collision);
    reg.register_tool("add_rectangle_collision", cmd_add_rectangle_collision);
}
} // namespace godot_mcp