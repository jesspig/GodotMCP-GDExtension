
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/navigation_region2d.hpp>
#include <godot_cpp/classes/navigation_region3d.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_polygon.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateNavigationRegionTool : public ITool {
public:
    String name() const noexcept override { return "create_navigation_region"; }
    String category() const noexcept override { return "editor_tools/navigation"; }
    String brief() const noexcept override {
        return "Create a NavigationRegion2D or NavigationRegion3D node";
    }
    String description() const override {
        return "Creates a NavigationRegion node with an associated navigation mesh resource. "
               "Supports 2D (NavigationPolygon) and 3D (NavigationMesh) with configurable properties.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("parent_path", "string", "Parent node path")
            .prop("dimension", "string", "Dimension: 2d or 3d", "3d")
            .prop("node_name", "string", "Node name (empty = auto)")
            .prop("cell_size", "number", "Cell size for the navigation mesh")
            .prop("agent_radius", "number", "Agent radius for the navigation mesh")
            .prop("agent_height", "number", "Agent height for the navigation mesh (3D only)")
            .required({"parent_path"})
            .build();
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String dimension = args_string(ctx.args, "dimension", "3d").to_lower();
        String node_name = args_string(ctx.args, "node_name");
        double cell_size = args_float(ctx.args, "cell_size", 0.0);
        double agent_radius = args_float(ctx.args, "agent_radius", 0.0);
        double agent_height = args_float(ctx.args, "agent_height", 0.0);

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        static const auto kNavRegionClasses = godot_mcp::make_dispatch_map<godot::String, godot::String>(
            std::pair{godot::String("2d"), godot::String("NavigationRegion2D")},
            std::pair{godot::String("3d"), godot::String("NavigationRegion3D")}
        );
        assert(kNavRegionClasses.validate() && "Duplicate key");

        const godot::String* matched = kNavRegionClasses.find(godot::String(dimension));
        if (!matched) {
            return ToolResult::err("INVALID_DIMENSION",
                String("Unknown dimension: ") + dimension + " (expected 2d/3d)");
        }
        godot::String class_name = *matched;
        godot::String default_name = class_name;
        bool is_3d = (dimension == "3d");

        Node *region_node = Object::cast_to<Node>(ClassDB::instantiate(class_name));
        if (!region_node) {
            return ToolResult::err("CREATE_FAILED", "Failed to create " + class_name);
        }
        MemdeleteGuard<Node> guard(region_node);

        if (node_name.is_empty()) {
            node_name = default_name;
        }
        region_node->set_name(node_name);

        if (is_3d) {
            godot::Ref<godot::NavigationMesh> nav_mesh;
            nav_mesh.instantiate();
            if (nav_mesh.is_null()) {
                return ToolResult::err("CREATE_FAILED", "Failed to create NavigationMesh resource");
            }
            if (cell_size > 0.0) {
                nav_mesh->set_cell_size(static_cast<real_t>(cell_size));
            }
            if (agent_radius > 0.0) {
                nav_mesh->set_agent_radius(static_cast<real_t>(agent_radius));
            }
            if (agent_height > 0.0) {
                nav_mesh->set_agent_height(static_cast<real_t>(agent_height));
            }
            auto *region = Object::cast_to<godot::NavigationRegion3D>(region_node);
            if (region) {
                region->set_navigation_mesh(nav_mesh);
            }
        } else {
            godot::Ref<godot::NavigationPolygon> nav_poly;
            nav_poly.instantiate();
            if (nav_poly.is_null()) {
                return ToolResult::err("CREATE_FAILED", "Failed to create NavigationPolygon resource");
            }
            if (cell_size > 0.0) {
                nav_poly->set_cell_size(static_cast<real_t>(cell_size));
            }
            if (agent_radius > 0.0) {
                nav_poly->set_agent_radius(static_cast<real_t>(agent_radius));
            }
            auto *region = Object::cast_to<godot::NavigationRegion2D>(region_node);
            if (region) {
                region->set_navigation_polygon(nav_poly);
            }
        }

        auto *ur = begin_undo_action("MCP: Create " + class_name, ctx.root);
        if (!ur) {
            parent->add_child(region_node, true, Node::INTERNAL_MODE_DISABLED);
            region_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            commit_add_child_undo(ur, "MCP: Create " + class_name, parent, region_node, ctx.root);
        }
        guard.dismiss();

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(region_node);
            }
        }

        Dictionary data;
        data["node_name"] = region_node->get_name();
        data["node_path"] = relative_path(ctx.root, region_node);
        data["class_name"] = class_name;
        data["dimension"] = is_3d ? "3d" : "2d";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
