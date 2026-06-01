// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/property_3d/property_3d_helpers.hpp"
#include <godot_cpp/variant/vector3.hpp>

namespace godot_mcp {

class SetNodeRotation3DTool : public ITool {
public:
    String name() const override { return "set_node_rotation_3d"; }
    String category() const override { return "property_3d"; }
    String brief() const override { return "Set a Node3D's rotation in degrees"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the 3D rotation_degrees (x, y, z) of a Node3D with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["rot_x"] = [](){Dictionary d;d["type"]="number";d["description"]="Rotation X";return d;}();
        p["rot_y"] = [](){Dictionary d;d["type"]="number";d["description"]="Rotation Y";return d;}();
        p["rot_z"] = [](){Dictionary d;d["type"]="number";d["description"]="Rotation Z";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String p = relative_path(ctx.root, ctx.node);
        Dictionary e = check_node3d(ctx.node, p); if (e.has("error")) return e;
        double x = args_float(ctx.args, "rot_x", args_float(ctx.args, "x", 0));
        double y = args_float(ctx.args, "rot_y", args_float(ctx.args, "y", 0));
        double z = args_float(ctx.args, "rot_z", args_float(ctx.args, "z", 0));
        undoable_set(ctx.node, "rotation_degrees", Vector3(x, y, z), "Set 3D rotation for " + p);
        Vector3 actual = ctx.node->get("rotation_degrees");
        Dictionary d; d["node_path"] = p; d["property"] = "rotation_degrees"; d["x"] = actual.x; d["y"] = actual.y; d["z"] = actual.z;
        return ToolResult::ok(d);
    }
};

} // namespace
