#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot_mcp {

inline Dictionary do_collision_shape(Node *target, Node *root, const Dictionary &args, bool circle) {
    double radius = args_float(args, "radius", 10);
    double w = args_float(args, "width", 20), h = args_float(args, "height", 100);

    Variant shape;
    if (circle) {
        Ref<CircleShape2D> cs; cs.instantiate(); cs->set_radius(radius); shape = cs;
    } else {
        Ref<RectangleShape2D> rs; rs.instantiate(); rs->set_size(Vector2(w, h)); shape = rs;
    }

    if (target->get_class() == "CollisionShape2D") {
        Variant old = target->get("shape");
        target->set("shape", shape);
        EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) { ur->create_action("Set collision shape"); ur->add_do_property(target, "shape", shape); ur->add_undo_property(target, "shape", old); ur->commit_action(false); }
        Dictionary d; d["node_path"] = relative_path(root, target); d["shape"] = circle ? "CircleShape2D" : "RectangleShape2D"; d["mode"] = "set_on_existing"; return d;
    }
    for (int64_t i = 0; i < target->get_child_count(); i++) {
        Node *child = Object::cast_to<Node>(target->get_child(i));
        if (child && child->get_class() == "CollisionShape2D") {
            Variant old = child->get("shape");
            child->set("shape", shape);
            EditorUndoRedoManager *ur = get_undo_redo();
            if (ur) { ur->create_action("Set collision shape"); ur->add_do_property(child, "shape", shape); ur->add_undo_property(child, "shape", old); ur->commit_action(false); }
            Dictionary d; d["node_path"] = relative_path(root, target); d["shape"] = circle ? "CircleShape2D" : "RectangleShape2D"; d["mode"] = "set_on_existing"; return d;
        }
    }
    Node *shape_node = Object::cast_to<Node>(ClassDB::instantiate("CollisionShape2D"));
    if (!shape_node) return ToolResult::err("INSTANTIATE_FAILED", "Failed to instantiate CollisionShape2D");
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
    Dictionary d; d["node_path"] = relative_path(root, target); d["shape"] = circle ? "CircleShape2D" : "RectangleShape2D"; d["mode"] = "created_child"; return d;
}

} // namespace godot_mcp
