
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/navigation_region3d.hpp>
#include <godot_cpp/classes/navigation_region2d.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_polygon.hpp>

namespace godot_mcp {

class BakeNavigationMeshTool : public ITool {
public:
    String name() const noexcept override { return "bake_navigation_mesh"; }
    String category() const noexcept override { return "editor_tools/navigation"; }
    String brief() const noexcept override {
        return "Bake navigation mesh for a NavigationRegion node";
    }
    String description() const override {
        return "Triggers navigation mesh baking on a NavigationRegion2D or "
               "NavigationRegion3D node. Baking can be computationally expensive.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("region_path", "string", "Path to the NavigationRegion node")
            .prop("timeout_seconds", "number", "Timeout in seconds for baking", 30.0)
            .required({"region_path"})
            .build();
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String region_path = args_string(ctx.args, "region_path");

        if (region_path.is_empty()) {
            return ToolResult::err("BAD_PARAM", "region_path is required");
        }

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, region_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        String node_class = node->get_class();
        bool is_3d = (node_class == "NavigationRegion3D");
        bool is_2d = (node_class == "NavigationRegion2D");

        if (!is_3d && !is_2d) {
            return ToolResult::err("INVALID_TYPE",
                "Node is not a NavigationRegion: " + node_class);
        }

        if (is_3d) {
            auto *region = Object::cast_to<godot::NavigationRegion3D>(node);
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

        auto *region = Object::cast_to<godot::NavigationRegion2D>(node);
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
