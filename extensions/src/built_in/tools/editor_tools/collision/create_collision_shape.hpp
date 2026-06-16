#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"


#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape2d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/classes/convex_polygon_shape2d.hpp>
#include <godot_cpp/classes/convex_polygon_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/separation_ray_shape2d.hpp>
#include <godot_cpp/classes/separation_ray_shape3d.hpp>
#include <godot_cpp/classes/segment_shape2d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/world_boundary_shape2d.hpp>
#include <godot_cpp/classes/world_boundary_shape3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot_mcp {

// 2D shape defaults
static constexpr auto kDefaultRectSize = 100.0f;
static constexpr auto kDefaultCircleRadius = 50.0f;
static constexpr auto kDefaultCapsuleRadius = 20.0f;
static constexpr auto kDefaultCapsuleHeight = 40.0f;

template<bool Is3D>
static godot::Node *create_body_node_impl(const String &body_type, const String &name) {
    static const auto kBodyClasses = godot_mcp::make_dispatch_map<godot::String, godot::String>(
        std::pair{godot::String("static"),    godot::String("StaticBody")},
        std::pair{godot::String("rigid"),     godot::String("RigidBody")},
        std::pair{godot::String("character"), godot::String("CharacterBody")},
        std::pair{godot::String("area"),      godot::String("Area")}
    );
    assert(kBodyClasses.validate() && "Duplicate key");

    const String suffix = Is3D ? "3D" : "2D";
    const godot::String* base = kBodyClasses.find(godot::String(body_type));
    if (!base) return nullptr;
    String class_name = *base + suffix;
    Node *node = godot::Object::cast_to<godot::Node>(godot::ClassDB::instantiate(class_name));
    if (node) node->set_name(name);
    return node;
}

static godot::Node *create_body_node(const String &body_type, const String &name) { return create_body_node_impl<false>(body_type, name); }
static godot::Node *create_body_node_3d(const String &body_type, const String &name) { return create_body_node_impl<true>(body_type, name); }

template<bool Is3D>
static godot::Ref<godot::Resource> create_shape_resource_impl(const String &shape_type, const Dictionary &props) {
    if constexpr (Is3D) {
        if (shape_type == "box") {
            godot::Ref<godot::BoxShape3D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("size")) {
                Dictionary s = props["size"];
                real_t x = static_cast<real_t>(args_float(s, "x", 1.0));
                real_t y = static_cast<real_t>(args_float(s, "y", 1.0));
                real_t z = static_cast<real_t>(args_float(s, "z", 1.0));
                shape->set_size(godot::Vector3(x, y, z));
            }
            return shape;
        }

        if (shape_type == "sphere") {
            godot::Ref<godot::SphereShape3D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("radius")) {
                shape->set_radius(static_cast<real_t>(args_float(props, "radius", 0.5)));
            }
            return shape;
        }

        if (shape_type == "capsule") {
            godot::Ref<godot::CapsuleShape3D> shape;
            shape.instantiate();
            if (!props.is_empty()) {
                if (props.has("radius"))
                    shape->set_radius(static_cast<real_t>(args_float(props, "radius", 0.5)));
                if (props.has("height"))
                    shape->set_height(static_cast<real_t>(args_float(props, "height", 1.0)));
            }
            return shape;
        }

        if (shape_type == "cylinder") {
            godot::Ref<godot::CylinderShape3D> shape;
            shape.instantiate();
            if (!props.is_empty()) {
                if (props.has("radius"))
                    shape->set_radius(static_cast<real_t>(args_float(props, "radius", 0.5)));
                if (props.has("height"))
                    shape->set_height(static_cast<real_t>(args_float(props, "height", 1.0)));
            }
            return shape;
        }

        if (shape_type == "convex_polygon") {
            godot::Ref<godot::ConvexPolygonShape3D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("points")) {
                godot::Array pts = props["points"];
                godot::PackedVector3Array points;
                for (int64_t i = 0; i < pts.size(); i++) {
                    godot::Array p = pts[i];
                    points.append(godot::Vector3(static_cast<real_t>(static_cast<double>(p[0])), static_cast<real_t>(static_cast<double>(p[1])), static_cast<real_t>(static_cast<double>(p[2]))));
                }
                shape->set_points(points);
            }
            return shape;
        }

        if (shape_type == "concave_polygon") {
            godot::Ref<godot::ConcavePolygonShape3D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("faces")) {
                godot::Array fs = props["faces"];
                godot::PackedVector3Array faces;
                for (int64_t i = 0; i < fs.size(); i++) {
                    godot::Array f = fs[i];
                    faces.append(godot::Vector3(static_cast<real_t>(static_cast<double>(f[0])), static_cast<real_t>(static_cast<double>(f[1])), static_cast<real_t>(static_cast<double>(f[2]))));
                }
                shape->set_faces(faces);
            }
            return shape;
        }

        if (shape_type == "height_map") {
            godot::Ref<godot::HeightMapShape3D> shape;
            shape.instantiate();
            if (!props.is_empty()) {
                if (props.has("map_width"))
                    shape->set_map_width(static_cast<int32_t>(args_int(props, "map_width", 2)));
                if (props.has("map_depth"))
                    shape->set_map_depth(static_cast<int32_t>(args_int(props, "map_depth", 2)));
                if (props.has("map_data")) {
                    godot::Array data = props["map_data"];
                    godot::PackedFloat32Array map_data;
                    for (int64_t i = 0; i < data.size(); i++) {
                        map_data.append(static_cast<float>(static_cast<double>(data[i])));
                    }
                    shape->set_map_data(map_data);
                }
            }
            return shape;
        }

        if (shape_type == "world_boundary") {
            godot::Ref<godot::WorldBoundaryShape3D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("plane")) {
                godot::Array pl = props["plane"];
                if (pl.size() >= 4) {
                    shape->set_plane(godot::Plane(
                        static_cast<real_t>(static_cast<double>(pl[0])), static_cast<real_t>(static_cast<double>(pl[1])),
                        static_cast<real_t>(static_cast<double>(pl[2])), static_cast<real_t>(static_cast<double>(pl[3]))));
                }
            }
            return shape;
        }

        if (shape_type == "separation_ray") {
            godot::Ref<godot::SeparationRayShape3D> shape;
            shape.instantiate();
            if (!props.is_empty()) {
                if (props.has("length"))
                    shape->set_length(static_cast<real_t>(args_float(props, "length", 1.0)));
                if (props.has("slide_on_slope"))
                    shape->set_slide_on_slope(args_bool(props, "slide_on_slope", false));
            }
            return shape;
        }
    } else {
        if (shape_type == "rectangle") {
            godot::Ref<godot::RectangleShape2D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("size")) {
                Dictionary s = props["size"];
                real_t w = static_cast<real_t>(args_float(s, "x", kDefaultRectSize));
                real_t h = static_cast<real_t>(args_float(s, "y", kDefaultRectSize));
                shape->set_size(godot::Vector2(w, h));
            }
            return shape;
        }

        if (shape_type == "circle") {
            godot::Ref<godot::CircleShape2D> shape;
            shape.instantiate();
            if (!props.is_empty() && props.has("radius")) {
                shape->set_radius(static_cast<real_t>(args_float(props, "radius", kDefaultCircleRadius)));
            }
            return shape;
        }

        if (shape_type == "capsule") {
            godot::Ref<godot::CapsuleShape2D> shape;
            shape.instantiate();
            if (!props.is_empty()) {
                if (props.has("radius"))
                    shape->set_radius(static_cast<real_t>(args_float(props, "radius", kDefaultCapsuleRadius)));
                if (props.has("height"))
                    shape->set_height(static_cast<real_t>(args_float(props, "height", kDefaultCapsuleHeight)));
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
                    points.append(godot::Vector2(static_cast<real_t>(static_cast<double>(p[0])), static_cast<real_t>(static_cast<double>(p[1]))));
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
                    points.append(godot::Vector2(static_cast<real_t>(static_cast<double>(s[0])), static_cast<real_t>(static_cast<double>(s[1]))));
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
                    shape->set_normal(godot::Vector2(static_cast<real_t>(static_cast<double>(n[0])), static_cast<real_t>(static_cast<double>(n[1]))));
                }
                if (props.has("distance"))
                    shape->set_distance(static_cast<real_t>(args_float(props, "distance", 0.0)));
            }
            return shape;
        }

        if (shape_type == "separation_ray") {
            godot::Ref<godot::SeparationRayShape2D> shape;
            shape.instantiate();
            if (!props.is_empty()) {
                if (props.has("length"))
                    shape->set_length(static_cast<real_t>(args_float(props, "length", 20.0)));
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
                    shape->set_a(godot::Vector2(static_cast<real_t>(static_cast<double>(a[0])), static_cast<real_t>(static_cast<double>(a[1]))));
                }
                if (props.has("b")) {
                    godot::Array b = props["b"];
                    shape->set_b(godot::Vector2(static_cast<real_t>(static_cast<double>(b[0])), static_cast<real_t>(static_cast<double>(b[1]))));
                }
            }
            return shape;
        }
    }

    return godot::Ref<godot::Resource>();
}

static godot::Ref<godot::Shape2D> create_shape_resource(const String &shape_type, const Dictionary &props) {
    auto r = create_shape_resource_impl<false>(shape_type, props);
    return godot::Ref<godot::Shape2D>(godot::Object::cast_to<godot::Shape2D>(r.ptr()));
}

static godot::Ref<godot::Shape3D> create_shape_resource_3d(const String &shape_type, const Dictionary &props) {
    auto r = create_shape_resource_impl<true>(shape_type, props);
    return godot::Ref<godot::Shape3D>(godot::Object::cast_to<godot::Shape3D>(r.ptr()));
}

class CreateCollisionShapeTool : public ITool {
public:
    String name() const noexcept override { return "create_collision_shape"; }
    String category() const noexcept override { return "editor_tools/collision"; }
    String brief() const noexcept override {
        return String("One-step create physics body + collision shape");
    }
    String description() const override {
        return String("Creates a physics body with a CollisionShape child and configurable shape resource. "
                            "Supports 2d (StaticBody2D, RigidBody2D, CharacterBody2D, Area2D with CollisionShape2D) "
                            "and 3d (StaticBody3D, RigidBody3D, CharacterBody3D, Area3D with CollisionShape3D). "
                            "2D shapes: rectangle, circle, capsule, convex_polygon, concave_polygon, "
                            "world_boundary, separation_ray, segment. "
                            "3D shapes: box, sphere, capsule, cylinder, convex_polygon, concave_polygon, "
                            "height_map, world_boundary, separation_ray.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Parent node path");
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Physics dimension: 2d or 3d (default: 2d)");
            p["default"] = "2d";
            props["dimension"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Body type: static/rigid/character/area");
            p["default"] = "static";
            props["body_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Shape type (2D: rectangle/circle/capsule/convex_polygon/concave_polygon/world_boundary/separation_ray/segment; 3D: box/sphere/capsule/cylinder/convex_polygon/concave_polygon/height_map/world_boundary/separation_ray)");
            props["shape_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = String("Shape-specific properties (e.g. {size: {x:100, y:100}} for rectangle)");
            props["shape_properties"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node name for the body");
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
        Dictionary shape_props = args_get_typed<Dictionary>(ctx.args, "shape_properties", Dictionary());
        String default_node_name = (dimension == "3d") ? "CollisionShape3D" : "CollisionShape2D";
        String node_name = args_string(ctx.args, "node_name", default_node_name);

        if (dimension != "2d" && dimension != "3d") {
            return ToolResult::err("UNSUPPORTED_DIMENSION", String("dimension must be '2d' or '3d', got: ") + dimension);
        }

        if (shape_type.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("shape_type is required"));
        }

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        String body_name = node_name;
        String shape_name = node_name + "_Shape";

        Node *body;
        if (dimension == "3d") {
            body = create_body_node_3d(body_type, body_name);
        } else {
            body = create_body_node(body_type, body_name);
        }
        if (!body) {
            return ToolResult::err("UNKNOWN_BODY_TYPE", String("Unknown body type: ") + body_type);
        }
        MemdeleteGuard<Node> body_guard(body);

        String shape_class = (dimension == "3d") ? "CollisionShape3D" : "CollisionShape2D";
        Node *shape_node = godot::Object::cast_to<godot::Node>(godot::ClassDB::instantiate(shape_class));
        if (shape_node) shape_node->set_name(shape_name);
        if (!shape_node) {
            return ToolResult::err("CREATE_FAILED", String("Failed to create ") + shape_class);
        }
        MemdeleteGuard<Node> shape_guard(shape_node);

        if (dimension == "3d") {
            godot::Ref<godot::Shape3D> shape_res = create_shape_resource_3d(shape_type, shape_props);
            if (shape_res.is_null()) {
                return ToolResult::err("UNKNOWN_SHAPE_TYPE", String("Unknown 3D shape type: ") + shape_type);
            }
            auto *cs = godot::Object::cast_to<godot::CollisionShape3D>(shape_node);
            cs->set_shape(shape_res);
        } else {
            godot::Ref<godot::Shape2D> shape_res = create_shape_resource(shape_type, shape_props);
            if (shape_res.is_null()) {
                return ToolResult::err("UNKNOWN_SHAPE_TYPE", String("Unknown 2D shape type: ") + shape_type);
            }
            auto *cs = godot::Object::cast_to<godot::CollisionShape2D>(shape_node);
            cs->set_shape(shape_res);
        }

        if (parent->has_node(String("./") + body_name)) {
            return ToolResult::err("NAME_CONFLICT", String("A node with the same name already exists: ") + body_name);
        }

        auto *ur = begin_undo_action("MCP: Create CollisionShape " + body_name);
        if (!ur) {
            parent->add_child(body, true, Node::INTERNAL_MODE_DISABLED);
            body->set_owner(ctx.root);
            body->add_child(shape_node, true, Node::INTERNAL_MODE_DISABLED);
            shape_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            ur->add_do_method(parent, "add_child", body, true, static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(body, "set_owner", ctx.root);
            ur->add_do_reference(body);
            ur->add_undo_reference(body);

            ur->add_do_method(body, "add_child", shape_node, true, static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(shape_node, "set_owner", ctx.root);
            ur->add_do_reference(shape_node);
            ur->add_undo_reference(shape_node);

            ur->add_undo_method(shape_node, "set_owner", Variant());
            ur->add_undo_method(body, "remove_child", shape_node);
            ur->add_undo_method(body, "set_owner", Variant());
            ur->add_undo_method(parent, "remove_child", body);

            commit_undo_action(ur);
        }
        body_guard.dismiss();
        shape_guard.dismiss();

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(body);
            }
        }

        Dictionary data;
        data["body_path"] = relative_path(ctx.root, body);
        data["body_type"] = body_type;
        data["dimension"] = dimension;
        data["shape_path"] = relative_path(ctx.root, shape_node);
        data["shape_type"] = shape_type;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
