
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/navigation_region3d.hpp>
#include <godot_cpp/classes/navigation_region2d.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_polygon.hpp>

namespace godot_mcp {

class BakeNavigationMeshTool : public ITool {
public:
    String name() const override { return "bake_navigation_mesh"; }
    String category() const override { return "editor_tools/navigation"; }
    String brief() const override {
        return "Bake navigation mesh for a NavigationRegion node";
    }
    String description() const override {
        return "Triggers navigation mesh baking on a NavigationRegion2D or "
               "NavigationRegion3D node. Baking can be computationally expensive.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the NavigationRegion node";
            props["region_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Timeout in seconds for baking";
            p["default"] = 30.0;
            props["timeout_seconds"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("region_path");
            s["required"] = req;
        }
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String region_path = args_string(ctx.args, "region_path");

        if (region_path.is_empty()) {
            return ToolResult::err("BAD_PARAM", "region_path is required");
        }

        Node *node = resolve_node(ctx.root, region_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND", "NavigationRegion not found: " + region_path);
        }

        String node_class = node->get_class();
        bool is_3d = (node_class == "NavigationRegion3D");
        bool is_2d = (node_class == "NavigationRegion2D");

        if (!is_3d && !is_2d) {
            return ToolResult::err("INVALID_TYPE",
                "Node is not a NavigationRegion: " + node_class);
        }

        if (is_3d) {
            godot::NavigationRegion3D *region = Object::cast_to<godot::NavigationRegion3D>(node);
            if (!region) {
                return ToolResult::err("CAST_FAILED", "Failed to cast to NavigationRegion3D");
            }
            godot::Ref<godot::NavigationMesh> nav_mesh = region->get_navigation_mesh();
            if (nav_mesh.is_null()) {
                return ToolResult::err("NO_MESH",
                    "NavigationRegion3D has no NavigationMesh resource assigned");
            }

            Variant result = region->call("bake_navigation_mesh", true);

            Dictionary data;
            data["region_path"] = relative_path(ctx.root, node);
            data["dimension"] = "3d";
            data["bake_triggered"] = true;
            if (result.get_type() != Variant::NIL) {
                data["result"] = result;
            }
            return ToolResult::ok(data);
        }

        godot::NavigationRegion2D *region = Object::cast_to<godot::NavigationRegion2D>(node);
        if (!region) {
            return ToolResult::err("CAST_FAILED", "Failed to cast to NavigationRegion2D");
        }
        godot::Ref<godot::NavigationPolygon> nav_poly = region->get_navigation_polygon();
        if (nav_poly.is_null()) {
            return ToolResult::err("NO_MESH",
                "NavigationRegion2D has no NavigationPolygon resource assigned");
        }

        Variant result = region->call("bake_navigation_polygon", true);

        Dictionary data;
        data["region_path"] = relative_path(ctx.root, node);
        data["dimension"] = "2d";
        data["bake_triggered"] = true;
        if (result.get_type() != Variant::NIL) {
            data["result"] = result;
        }
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
