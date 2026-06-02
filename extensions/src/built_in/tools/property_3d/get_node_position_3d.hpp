// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/property_3d/property_3d_helpers.hpp"
#include <godot_cpp/variant/vector3.hpp>

namespace godot_mcp {

class GetNodePosition3DTool : public ITool {
public:
    String name() const override { return "get_node_position_3d"; }
    String category() const override { return "property/3d"; }
    String brief() const override { return "Get a Node3D's position"; }
    String category_description() const override { return String::utf8("3D 节点属性的读写操作"); }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }
    String description() const override { return "Gets the 3D position (x, y, z) of a Node3D."; }
    Dictionary input_schema() const override {
        Dictionary s; s["type"] = "object"; Dictionary p; p["node_path"] = [](){Dictionary d;d["type"]="string";d["description"]="Node path";return d;}(); s["properties"] = p;
        Array r; r.push_back("node_path"); s["required"] = r; return s;
    }
protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String p = relative_path(ctx.root, ctx.node);
        Dictionary e = check_node3d(ctx.node, p); if (e.has("error")) return e;
        Vector3 v = ctx.node->get("position");
        Dictionary d; d["node_path"] = p; d["x"] = v.x; d["y"] = v.y; d["z"] = v.z;
        return ToolResult::ok(d);
    }
};

} // namespace
