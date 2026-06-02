// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/property_3d/property_3d_helpers.hpp"
#include <godot_cpp/variant/vector3.hpp>

namespace godot_mcp {

class SetNodePosition3DTool : public ITool {
public:
    String name() const override { return "set_node_position_3d"; }
    String category() const override { return "property/3d"; }
    String brief() const override { return "Set a Node3D's position"; }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Sets the 3D position (x, y, z) of a Node3D with undo."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p;
        p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}();
        p["x"] = [](){Dictionary d;d["type"]="number";d["description"]="X";return d;}();
        p["y"] = [](){Dictionary d;d["type"]="number";d["description"]="Y";return d;}();
        p["z"] = [](){Dictionary d;d["type"]="number";d["description"]="Z";return d;}();
        s["properties"] = p; Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String p = relative_path(ctx.root, ctx.node);
        Dictionary e = check_node3d(ctx.node, p); if (e.has("error")) return e;
        double x = args_float(ctx.args, "x", 0), y = args_float(ctx.args, "y", 0), z = args_float(ctx.args, "z", 0);
        undoable_set(ctx.node, "position", Vector3(x, y, z), "Set 3D position for " + p);
        Vector3 actual = ctx.node->get("position");
        Dictionary d; d["node_path"] = p; d["property"] = "position"; d["x"] = actual.x; d["y"] = actual.y; d["z"] = actual.z;
        return ToolResult::ok(d);
    }
};

} // namespace
