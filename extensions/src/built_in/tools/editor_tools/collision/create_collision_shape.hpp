#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"


#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/concave_polygon_shape2d.hpp>
#include <godot_cpp/classes/convex_polygon_shape2d.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/separation_ray_shape2d.hpp>
#include <godot_cpp/classes/segment_shape2d.hpp>
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/world_boundary_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot_mcp {

static godot::Node *create_body_node(const String &body_type, const String &name) {
    String class_name;
    if (body_type == "static") class_name = "StaticBody2D";
    else if (body_type == "rigid") class_name = "RigidBody2D";
    else if (body_type == "character") class_name = "CharacterBody2D";
    else if (body_type == "area") class_name = "Area2D";
    else return nullptr;
    Node *node = godot::Object::cast_to<godot::Node>(godot::ClassDB::instantiate(class_name));
    if (node) node->set_name(name);
    return node;
}

static godot::Ref<godot::Shape2D> create_shape_resource(const String &shape_type, const Dictionary &props) {
    if (shape_type == "rectangle") {
        godot::Ref<godot::RectangleShape2D> shape;
        shape.instantiate();
        if (!props.is_empty() && props.has("size")) {
            Dictionary s = props["size"];
            real_t w = (real_t)args_float(s, "x", 100.0);
            real_t h = (real_t)args_float(s, "y", 100.0);
            shape->set_size(godot::Vector2(w, h));
        }
        return shape;
    }

    if (shape_type == "circle") {
        godot::Ref<godot::CircleShape2D> shape;
        shape.instantiate();
        if (!props.is_empty() && props.has("radius")) {
            shape->set_radius((real_t)args_float(props, "radius", 50.0));
        }
        return shape;
    }

    if (shape_type == "capsule") {
        godot::Ref<godot::CapsuleShape2D> shape;
        shape.instantiate();
        if (!props.is_empty()) {
            if (props.has("radius"))
                shape->set_radius((real_t)args_float(props, "radius", 20.0));
            if (props.has("height"))
                shape->set_height((real_t)args_float(props, "height", 40.0));
        }
        return shape;
    }

    if (shape_type == "convex_polygon") {
        godot::Ref<godot::ConvexPolygonShape2D> shape;
        shape.instantiate();
        if (!props.is_empty() && props.has("points")) {
            godot::Array pts = props["points"];
            godot::PackedVector2Array points;
            for (int64_t i = 0; i < pts.size(); i++) {
                godot::Array p = pts[i];
                points.append(godot::Vector2((real_t)(double)p[0], (real_t)(double)p[1]));
            }
            shape->set_points(points);
        }
        return shape;
    }

    if (shape_type == "concave_polygon") {
        godot::Ref<godot::ConcavePolygonShape2D> shape;
        shape.instantiate();
        if (!props.is_empty() && props.has("segments")) {
            godot::Array segs = props["segments"];
            godot::PackedVector2Array points;
            for (int64_t i = 0; i < segs.size(); i++) {
                godot::Array s = segs[i];
                points.append(godot::Vector2((real_t)(double)s[0], (real_t)(double)s[1]));
            }
            shape->set_segments(points);
        }
        return shape;
    }

    if (shape_type == "world_boundary") {
        godot::Ref<godot::WorldBoundaryShape2D> shape;
        shape.instantiate();
        if (!props.is_empty()) {
            if (props.has("normal")) {
                godot::Array n = props["normal"];
                shape->set_normal(godot::Vector2((real_t)(double)n[0], (real_t)(double)n[1]));
            }
            if (props.has("distance"))
                shape->set_distance((real_t)args_float(props, "distance", 0.0));
        }
        return shape;
    }

    if (shape_type == "separation_ray") {
        godot::Ref<godot::SeparationRayShape2D> shape;
        shape.instantiate();
        if (!props.is_empty()) {
            if (props.has("length"))
                shape->set_length((real_t)args_float(props, "length", 20.0));
            if (props.has("slide_on_slope"))
                shape->set_slide_on_slope(args_bool(props, "slide_on_slope", false));
        }
        return shape;
    }

    if (shape_type == "segment") {
        godot::Ref<godot::SegmentShape2D> shape;
        shape.instantiate();
        if (!props.is_empty()) {
            if (props.has("a")) {
                godot::Array a = props["a"];
                shape->set_a(godot::Vector2((real_t)(double)a[0], (real_t)(double)a[1]));
            }
            if (props.has("b")) {
                godot::Array b = props["b"];
                shape->set_b(godot::Vector2((real_t)(double)b[0], (real_t)(double)b[1]));
            }
        }
        return shape;
    }

    return godot::Ref<godot::Shape2D>();
}

class CreateCollisionShapeTool : public ITool {
public:
    String name() const override { return "create_collision_shape"; }
    String category() const override { return "editor_tools/collision"; }
    String brief() const override {
        return String::utf8("One-step create physics body + collision shape");
    }
    String description() const override {
        return String::utf8("Creates a physics body (StaticBody2D, RigidBody2D, CharacterBody2D, Area2D) "
                            "with a CollisionShape2D child and configurable shape resource. "
                            "Supports rectangle, circle, capsule, convex_polygon, concave_polygon, "
                            "world_boundary, separation_ray, and segment shapes.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Parent node path");
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Physics dimension (default: 2d)");
            p["default"] = "2d";
            props["dimension"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Body type: static/rigid/character/area");
            p["default"] = "static";
            props["body_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Shape type: rectangle/circle/capsule/convex_polygon/concave_polygon/world_boundary/separation_ray/segment");
            props["shape_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = String::utf8("Shape-specific properties (e.g. {size: {x:100, y:100}} for rectangle)");
            props["shape_properties"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Node name for the body");
            p["default"] = "CollisionShape2D";
            props["node_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("parent_path", "shape_type");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String dimension = args_string(ctx.args, "dimension", "2d");
        String body_type = args_string(ctx.args, "body_type", "static");
        String shape_type = args_string(ctx.args, "shape_type");
        Dictionary shape_props;
        if (ctx.args.has("shape_properties")) {
            shape_props = ctx.args["shape_properties"];
        }
        String node_name = args_string(ctx.args, "node_name", "CollisionShape2D");

        if (dimension != "2d") {
            return ToolResult::err("UNSUPPORTED_DIMENSION", String::utf8("Only 2d is supported currently"));
        }

        if (shape_type.is_empty()) {
            return ToolResult::err("MISSING_ARG", String::utf8("shape_type is required"));
        }

        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("NODE_NOT_FOUND", String::utf8("Parent node not found: ") + parent_path);
        }

        String body_name = node_name;
        String shape_name = node_name + "_Shape";

        // Create body
        Node *body = create_body_node(body_type, body_name);
        if (!body) {
            return ToolResult::err("UNKNOWN_BODY_TYPE", String::utf8("Unknown body type: ") + body_type);
        }

        // Create collision shape node
        Node *shape_node = godot::Object::cast_to<godot::Node>(godot::ClassDB::instantiate("CollisionShape2D"));
        if (shape_node) shape_node->set_name(shape_name);
        if (!shape_node) {
            memdelete(body);
            return ToolResult::err("CREATE_FAILED", String::utf8("Failed to create CollisionShape2D"));
        }
        godot::CollisionShape2D *cs = godot::Object::cast_to<godot::CollisionShape2D>(shape_node);

        // Create shape resource
        godot::Ref<godot::Shape2D> shape_res = create_shape_resource(shape_type, shape_props);
        if (shape_res.is_null()) {
            memdelete(body);
            memdelete(shape_node);
            return ToolResult::err("UNKNOWN_SHAPE_TYPE", String::utf8("Unknown shape type: ") + shape_type);
        }
        cs->set_shape(shape_res);

        // Check name conflicts
        if (parent->has_node(String("./") + body_name)) {
            memdelete(body);
            memdelete(shape_node);
            return ToolResult::err("NAME_CONFLICT", String::utf8("A node with the same name already exists: ") + body_name);
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        ur->create_action(String::utf8("MCP: Create CollisionShape ") + body_name,
                          godot::UndoRedo::MERGE_DISABLE, ctx.root);

        // Add body to parent first so it's in tree
        ur->add_do_method(parent, "add_child", body, true, (int64_t)Node::INTERNAL_MODE_DISABLED);
        ur->add_do_method(body, "set_owner", ctx.root);
        ur->add_do_reference(body);

        // Then add shape to body (body is now in tree)
        ur->add_do_method(body, "add_child", shape_node, true, (int64_t)Node::INTERNAL_MODE_DISABLED);
        ur->add_do_method(shape_node, "set_owner", ctx.root);
        ur->add_do_reference(shape_node);

        // Undo in reverse order
        ur->add_undo_method(shape_node, "set_owner", Variant());
        ur->add_undo_method(body, "remove_child", shape_node);
        ur->add_undo_method(body, "set_owner", Variant());
        ur->add_undo_method(parent, "remove_child", body);

        ur->commit_action();

        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            godot::EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(body);
            }
        }

        Dictionary data;
        data["body_path"] = relative_path(ctx.root, body);
        data["body_type"] = body_type;
        data["shape_path"] = relative_path(ctx.root, shape_node);
        data["shape_type"] = shape_type;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
