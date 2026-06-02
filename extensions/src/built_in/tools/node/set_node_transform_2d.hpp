// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/variant/vector2.hpp>

namespace godot_mcp {

class SetNodeTransform2DTool : public ITool {
public:
    String name() const override { return "set_node_transform_2d"; }
    String category() const override { return "node"; }
    String brief() const override { return "Set 2D transform (position, rotation, scale)"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Sets position, rotation_degrees, and scale on a Node2D. Omitted axes keep their current values.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node"; return d; }();
        props["x"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "X position (default: current)"; return d; }();
        props["y"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Y position (default: current)"; return d; }();
        props["degrees"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Rotation in degrees (default: current)"; return d; }();
        props["scale_x"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Scale X (default: current)"; return d; }();
        props["scale_y"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Scale Y (default: current)"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *n = ctx.node;
        Vector2 old_pos = n->get("position");
        double old_rot = n->get("rotation_degrees");
        Vector2 old_scl = n->get("scale");

        double x = args_float(ctx.args, "x", old_pos.x);
        double y = args_float(ctx.args, "y", old_pos.y);
        double rot = args_float(ctx.args, "degrees", old_rot);
        double sx = args_float(ctx.args, "scale_x", old_scl.x);
        double sy = args_float(ctx.args, "scale_y", old_scl.y);

        String p = relative_path(ctx.root, n);
        undoable_set(n, "position", Vector2(x, y), "Set 2D Transform for " + p);
        undoable_set(n, "rotation_degrees", rot, "Set 2D Transform for " + p);
        undoable_set(n, "scale", Vector2(sx, sy), "Set 2D Transform for " + p);

        Dictionary data;
        data["node_path"] = p;
        data["x"] = x; data["y"] = y;
        data["degrees"] = rot;
        data["scale_x"] = sx; data["scale_y"] = sy;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
