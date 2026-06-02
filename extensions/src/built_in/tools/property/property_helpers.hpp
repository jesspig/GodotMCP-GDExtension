#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

inline Dictionary check_collision_compat(Node *n, const String &p) {
    String cls = n->get_class();
    if (cls == "CollisionObject2D" || cls == "CollisionObject3D" || cls.begins_with("CharacterBody2D") ||
        cls.begins_with("CharacterBody3D") || cls.begins_with("Area2D") || cls.begins_with("Area3D") ||
        cls.begins_with("StaticBody") || cls.begins_with("RigidBody") || cls.begins_with("AnimatableBody"))
        return Dictionary();
    return ToolResult::err("INCOMPATIBLE", "Node '" + p + "' (" + cls + ") does not have collision_layer/mask");
}

} // namespace godot_mcp
