#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation_node.hpp>
#include <godot_cpp/classes/animation_node_animation.hpp>
#include <godot_cpp/classes/animation_node_blend2.hpp>
#include <godot_cpp/classes/animation_node_blend3.hpp>
#include <godot_cpp/classes/animation_node_blend_tree.hpp>
#include <godot_cpp/classes/animation_node_one_shot.hpp>
#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_time_seek.hpp>
#include <godot_cpp/classes/animation_node_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class AddAnimationNodeTool : public ITool {
public:
    String name() const override { return "add_animation_node"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Add an animation node to an AnimationTree state machine";
    }
    String description() const override {
        return "Adds an AnimationNode subclass (animation, blend2, blend3, blend_tree, "
               "one_shot, time_seek, transition) to the root AnimationNodeStateMachine "
               "of an AnimationTree. Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the AnimationTree node";
            props["tree_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node type: animation/blend2/blend3/blend_tree/one_shot/time_seek/transition";
            props["node_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Name for the new node in the state machine";
            props["name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Position in graph {x, y}";
            props["position"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Animation clip name (for 'animation' node type)";
            props["animation_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("tree_path", "node_type", "name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String tree_path = args_string(ctx.args, "tree_path");
        String node_type = args_string(ctx.args, "node_type");
        String node_name = args_string(ctx.args, "name");
        String animation_name = args_string(ctx.args, "animation_name", "");

        godot::Vector2 position;
        if (ctx.args.has("position") && ctx.args["position"].get_type() == Variant::DICTIONARY) {
            Dictionary pos = ctx.args["position"];
            position.x = (real_t)args_float(pos, "x", 0.0);
            position.y = (real_t)args_float(pos, "y", 0.0);
        }

        Node *node = resolve_node(ctx.root, tree_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("AnimationTree not found: ") + tree_path);
        }
        godot::AnimationTree *tree = Object::cast_to<godot::AnimationTree>(node);
        if (!tree) {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an AnimationTree: ") + tree_path);
        }

        godot::Ref<godot::AnimationRootNode> root = tree->get_tree_root();
        if (root.is_null()) {
            return ToolResult::err("NO_ROOT", "AnimationTree has no root node");
        }

        godot::Ref<godot::AnimationNodeStateMachine> sm =
            Object::cast_to<godot::AnimationNodeStateMachine>(root.ptr());
        if (sm.is_null()) {
            return ToolResult::err("NOT_STATE_MACHINE",
                "Tree root is not an AnimationNodeStateMachine");
        }

        if (sm->has_node(godot::StringName(node_name))) {
            return ToolResult::err("NAME_CONFLICT",
                String("State already exists: ") + node_name);
        }

        String class_name;
        if (node_type == "animation") class_name = "AnimationNodeAnimation";
        else if (node_type == "blend2") class_name = "AnimationNodeBlend2";
        else if (node_type == "blend3") class_name = "AnimationNodeBlend3";
        else if (node_type == "blend_tree") class_name = "AnimationNodeBlendTree";
        else if (node_type == "one_shot") class_name = "AnimationNodeOneShot";
        else if (node_type == "time_seek") class_name = "AnimationNodeTimeSeek";
        else if (node_type == "transition") class_name = "AnimationNodeTransition";
        else {
            return ToolResult::err("INVALID_NODE_TYPE",
                String("Unknown node_type: ") + node_type);
        }

        godot::Ref<godot::AnimationNode> anim_node =
            Object::cast_to<godot::AnimationNode>(
                godot::ClassDB::instantiate(class_name));
        if (anim_node.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                String("Failed to create ") + class_name);
        }

        if (node_type == "animation" && !animation_name.is_empty()) {
            godot::AnimationNodeAnimation *anim =
                Object::cast_to<godot::AnimationNodeAnimation>(anim_node.ptr());
            if (anim) {
                anim->set_animation(godot::StringName(animation_name));
            }
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            sm->add_node(godot::StringName(node_name), anim_node, position);
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Add AnimationNode ") + node_name,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(sm.ptr(), "add_node",
                              godot::StringName(node_name), anim_node, position);
            ur->add_undo_method(sm.ptr(), "remove_node",
                                godot::StringName(node_name));
            ur->commit_action();
        }

        Dictionary data;
        data["node_name"] = node_name;
        data["node_type"] = node_type;
        data["class_name"] = class_name;
        data["animation_name"] = animation_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
