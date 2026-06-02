#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

inline Dictionary check_node3d(Node *n, const String &p) {
    if (n->is_class("Node3D")) return Dictionary();
    return ToolResult::err("NOT_3D", "Node '" + p + "' (" + n->get_class() + ") is not a Node3D");
}

} // namespace godot_mcp
