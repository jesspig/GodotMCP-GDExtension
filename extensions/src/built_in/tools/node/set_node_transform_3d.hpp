// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/variant/vector3.hpp>

namespace godot_mcp {

class SetNodeTransform3DTool : public ITool {
public:
    String name() const override { return "set_node_transform_3d"; }
    String category() const override { return "node"; }
    String brief() const override { return "Set 3D transform (position, rotation, scale)"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override {
        return "Sets position, rotation_degrees, and scale on a Node3D. Omitted axes keep their current values.";
    }
    Dictionary input_schema() const override {
        Dictionary schema;
        schema["type"] = "object";
        Dictionary props;
        props["node_path"] = []() { Dictionary d; d["type"] = "string"; d["description"] = "Path to the node"; return d; }();
        props["x"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "X position"; return d; }();
        props["y"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Y position"; return d; }();
        props["z"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Z position"; return d; }();
        props["rot_x"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Rotation X in degrees"; return d; }();
        props["rot_y"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Rotation Y in degrees"; return d; }();
        props["rot_z"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Rotation Z in degrees"; return d; }();
        props["scale_x"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Scale X"; return d; }();
        props["scale_y"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Scale Y"; return d; }();
        props["scale_z"] = []() { Dictionary d; d["type"] = "number"; d["description"] = "Scale Z"; return d; }();
        schema["properties"] = props;
        Array req; req.push_back("node_path"); schema["required"] = req;
        return schema;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        Node *n = ctx.node;
        if (!n->is_class("Node3D")) return ToolResult::err("NOT_3D", "Node is not Node3D");

        Vector3 old_pos = n->get("position");
        Vector3 old_rot = n->get("rotation_degrees");
        Vector3 old_scl = n->get("scale");

        double x = args_float(ctx.args, "x", old_pos.x);
        double y = args_float(ctx.args, "y", old_pos.y);
        double z = args_float(ctx.args, "z", old_pos.z);
        double rx = args_float(ctx.args, "rot_x", old_rot.x);
        double ry = args_float(ctx.args, "rot_y", old_rot.y);
        double rz = args_float(ctx.args, "rot_z", old_rot.z);
        double sx = args_float(ctx.args, "scale_x", old_scl.x);
        double sy = args_float(ctx.args, "scale_y", old_scl.y);
        double sz = args_float(ctx.args, "scale_z", old_scl.z);

        String p = relative_path(ctx.root, n);
        undoable_set(n, "position", Vector3(x, y, z), "Set 3D Transform for " + p);
        undoable_set(n, "rotation_degrees", Vector3(rx, ry, rz), "Set 3D Transform for " + p);
        undoable_set(n, "scale", Vector3(sx, sy, sz), "Set 3D Transform for " + p);

        Dictionary data;
        data["node_path"] = p;
        data["x"] = x; data["y"] = y; data["z"] = z;
        data["rot_x"] = rx; data["rot_y"] = ry; data["rot_z"] = rz;
        data["scale_x"] = sx; data["scale_y"] = sy; data["scale_z"] = sz;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
