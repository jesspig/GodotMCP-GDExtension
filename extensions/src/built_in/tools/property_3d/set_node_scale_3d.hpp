// @tool register
// @source builtin
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/property_3d/property_3d_helpers.hpp"
#include <godot_cpp/variant/vector3.hpp>

namespace godot_mcp {

class SetNodeScale3DTool : public ITool {
public:
    String name() const override { return "set_node_scale_3d"; }
    String category() const override { return "property_3d"; }
    String brief() const override { return "Set a Node3D's scale"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the 3D scale (x, y, z) of a Node3D with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["scale_x"] = [](){Dictionary d;d["type"]="number";d["description"]="Scale X";return d;}();
        p["scale_y"] = [](){Dictionary d;d["type"]="number";d["description"]="Scale Y";return d;}();
        p["scale_z"] = [](){Dictionary d;d["type"]="number";d["description"]="Scale Z";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String p = relative_path(ctx.root, ctx.node);
        Dictionary e = check_node3d(ctx.node, p); if (e.has("error")) return e;
        double x = args_float(ctx.args, "scale_x", args_float(ctx.args, "x", 1));
        double y = args_float(ctx.args, "scale_y", args_float(ctx.args, "y", 1));
        double z = args_float(ctx.args, "scale_z", args_float(ctx.args, "z", 1));
        undoable_set(ctx.node, "scale", Vector3(x, y, z), "Set 3D scale for " + p);
        Vector3 actual = ctx.node->get("scale");
        Dictionary d; d["node_path"] = p; d["property"] = "scale"; d["x"] = actual.x; d["y"] = actual.y; d["z"] = actual.z;
        return ToolResult::ok(d);
    }
};

} // namespace
