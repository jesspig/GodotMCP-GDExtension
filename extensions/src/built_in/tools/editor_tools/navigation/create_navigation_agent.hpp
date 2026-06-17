
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateNavigationAgentTool : public ITool {
public:
    String name() const noexcept override { return "create_navigation_agent"; }
    String category() const noexcept override { return "editor_tools/navigation"; }
    String brief() const noexcept override {
        return "Create a NavigationAgent2D or NavigationAgent3D node";
    }
    String description() const override {
        return "Creates a NavigationAgent node for pathfinding. "
               "Supports optional configuration of target/path distances and avoidance.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("parent_path", "string", "Parent node path")
            .prop("dimension", "string", "Dimension: 2d or 3d", "3d")
            .prop("node_name", "string", "Node name (empty = auto)")
            .prop("target_desired_distance", "number", "Target desired distance")
            .prop("path_desired_distance", "number", "Path desired distance")
            .prop("avoidance_enabled", "boolean", "Enable avoidance")
            .required({"parent_path"})
            .build();
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String dimension = args_string(ctx.args, "dimension", "3d").to_lower();
        String node_name = args_string(ctx.args, "node_name");
        double target_dist = args_float(ctx.args, "target_desired_distance", 0.0);
        double path_dist = args_float(ctx.args, "path_desired_distance", 0.0);
        bool avoidance = args_bool(ctx.args, "avoidance_enabled", false);

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        bool is_3d = (dimension != "2d");
        String class_name = is_3d ? "NavigationAgent3D" : "NavigationAgent2D";
        String default_name = is_3d ? "NavigationAgent3D" : "NavigationAgent2D";

        Node *agent_node = Object::cast_to<Node>(ClassDB::instantiate(class_name));
        if (!agent_node) {
            return ToolResult::err("CREATE_FAILED", "Failed to create " + class_name);
        }

        if (node_name.is_empty()) {
            node_name = default_name;
        }
        agent_node->set_name(node_name);

        if (target_dist > 0.0) {
            agent_node->set("target_desired_distance", static_cast<real_t>(target_dist));
        }
        if (path_dist > 0.0) {
            agent_node->set("path_desired_distance", static_cast<real_t>(path_dist));
        }
        if (avoidance) {
            agent_node->set("avoidance_enabled", true);
        }

        auto *ur = begin_undo_action("MCP: Create " + class_name);
        if (!ur) {
            parent->add_child(agent_node, true, Node::INTERNAL_MODE_DISABLED);
            agent_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            commit_add_child_undo(ur, "MCP: Create " + class_name, parent, agent_node, ctx.root);
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(agent_node);
            }
        }

        Dictionary data;
        data["node_name"] = agent_node->get_name();
        data["node_path"] = relative_path(ctx.root, agent_node);
        data["class_name"] = class_name;
        data["dimension"] = is_3d ? "3d" : "2d";
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
