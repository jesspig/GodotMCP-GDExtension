
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateNavigationAgentTool : public ITool {
public:
    String name() const override { return "create_navigation_agent"; }
    String category() const override { return "editor_tools/navigation"; }
    String brief() const override {
        return "Create a NavigationAgent2D or NavigationAgent3D node";
    }
    String description() const override {
        return "Creates a NavigationAgent node for pathfinding. "
               "Supports optional configuration of target/path distances and avoidance.";
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
            p["description"] = "Target desired distance";
            props["target_desired_distance"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Path desired distance";
            props["path_desired_distance"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Enable avoidance";
            props["avoidance_enabled"] = p;
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
        double target_dist = args_float(ctx.args, "target_desired_distance", 0.0);
        double path_dist = args_float(ctx.args, "path_desired_distance", 0.0);
        bool avoidance = args_bool(ctx.args, "avoidance_enabled", false);

        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("NODE_NOT_FOUND", "Parent node not found: " + parent_path);
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

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->add_child(agent_node, true, Node::INTERNAL_MODE_DISABLED);
            agent_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Create ") + class_name,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "add_child", agent_node, true,
                              static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(agent_node, "set_owner", ctx.root);
            ur->add_undo_method(parent, "remove_child", agent_node);
            ur->add_do_reference(agent_node);
            ur->commit_action();
        }

        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            godot::EditorSelection *sel = ei->get_selection();
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
