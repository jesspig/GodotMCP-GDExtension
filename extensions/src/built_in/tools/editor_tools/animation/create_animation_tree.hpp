
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateAnimationTreeTool : public ITool {
public:
    String name() const noexcept override { return "create_animation_tree"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Create an AnimationTree node with a root state machine";
    }
    String description() const override {
        return "Creates an AnimationTree node as a child of the specified parent, "
               "connects it to an AnimationPlayer, and creates a root AnimationNodeStateMachine. "
               "Uses EditorUndoRedoManager for undo support.";
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
            p["description"] = "Path to the AnimationPlayer node to connect";
            props["anim_player_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name for the AnimationTree";
            p["default"] = "AnimationTree";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Root node type (currently only state_machine)";
            p["default"] = "state_machine";
            props["root_type"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("parent_path", "anim_player_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String anim_player_path = args_string(ctx.args, "anim_player_path");
        String node_name = args_string(ctx.args, "node_name", "AnimationTree");
        String root_type = args_string(ctx.args, "root_type", "state_machine");

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        Node *ap_node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, anim_player_path, ap_node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        auto *player = Object::cast_to<godot::AnimationPlayer>(ap_node);
        if (!player) {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an AnimationPlayer: ") + anim_player_path);
        }

        if (root_type != "state_machine") {
            return ToolResult::err("INVALID_ROOT_TYPE",
                String("Unsupported root_type: ") + root_type + " (only 'state_machine' is supported)");
        }

        Node *tree_node = Object::cast_to<Node>(ClassDB::instantiate("AnimationTree"));
        if (!tree_node) {
            return ToolResult::err("CREATE_FAILED", "Failed to create AnimationTree node");
        }
        MemdeleteGuard<Node> guard(tree_node);
        tree_node->set_name(node_name);

        auto *tree = Object::cast_to<godot::AnimationTree>(tree_node);
        if (!tree) {
            return ToolResult::err("CAST_FAILED", "Failed to cast node to AnimationTree");
        }

        godot::NodePath player_path = tree->get_path_to(player);
        tree->set_animation_player(player_path);

        godot::Ref<godot::AnimationNodeStateMachine> sm;
        sm.instantiate();
        if (sm.is_null()) {
            return ToolResult::err("CREATE_FAILED", "Failed to create AnimationNodeStateMachine");
        }
        tree->set_tree_root(sm);

        if (parent->has_node(String("./") + node_name)) {
            return ToolResult::err("NAME_CONFLICT",
                String("A node with the same name already exists: ") + node_name);
        }

        auto *ur = begin_undo_action("MCP: Create AnimationTree");
        if (!ur) {
            parent->add_child(tree_node, true, Node::INTERNAL_MODE_DISABLED);
            tree_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            commit_add_child_undo(ur, "MCP: Create AnimationTree", parent, tree_node, ctx.root);
        }
        guard.dismiss();

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(tree_node);
            }
        }

        Dictionary data;
        data["node_name"] = tree_node->get_name();
        data["node_path"] = relative_path(ctx.root, tree_node);
        data["animation_player"] = anim_player_path;
        data["root_type"] = root_type;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
