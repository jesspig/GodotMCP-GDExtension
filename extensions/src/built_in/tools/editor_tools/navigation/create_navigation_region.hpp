
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
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
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Dimension: 2d or 3d";
            p["default"] = "3d";
            props["dimension"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name (empty = auto)";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Cell size for the navigation mesh";
            props["cell_size"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Agent radius for the navigation mesh";
            props["agent_radius"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Agent height for the navigation mesh (3D only)";
            props["agent_height"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        {
            Array req;
            req.append("parent_path");
            s["required"] = req;
        }
        return s;
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

        bool is_3d = (dimension != "2d");
        String class_name = is_3d ? "NavigationRegion3D" : "NavigationRegion2D";
        String default_name = is_3d ? "NavigationRegion3D" : "NavigationRegion2D";

        Node *region_node = Object::cast_to<Node>(ClassDB::instantiate(class_name));
        if (!region_node) {
            return ToolResult::err("CREATE_FAILED", "Failed to create " + class_name);
        }

        if (node_name.is_empty()) {
            node_name = default_name;
        }
        region_node->set_name(node_name);

        if (is_3d) {
            godot::Ref<godot::NavigationMesh> nav_mesh;
            nav_mesh.instantiate();
            if (nav_mesh.is_null()) {
                memdelete(region_node);
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
                memdelete(region_node);
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

        auto *ur = begin_undo_action("MCP: Create " + class_name);
        if (!ur) {
            parent->add_child(region_node, true, Node::INTERNAL_MODE_DISABLED);
            region_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            ur->add_do_method(parent, "add_child", region_node, true,
                              static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(region_node, "set_owner", ctx.root);
            ur->add_undo_method(parent, "remove_child", region_node);
            ur->add_undo_reference(region_node);
            commit_undo_action(ur);
        }

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
