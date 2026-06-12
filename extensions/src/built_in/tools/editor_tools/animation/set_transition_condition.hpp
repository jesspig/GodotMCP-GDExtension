#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_state_machine_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class SetTransitionConditionTool : public ITool {
public:
    String name() const override { return "set_transition_condition"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Set a condition on a state machine transition";
    }
    String description() const override {
        return "Sets or clears a condition name on a transition between two states "
               "in the root AnimationNodeStateMachine. The condition controls when "
               "the transition fires. Uses EditorUndoRedoManager for undo support.";
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
            p["description"] = "Source state name";
            props["from"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target state name";
            props["to"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Condition name to set on the transition";
            props["condition"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Whether to set or clear the condition (default: true)";
            p["default"] = true;
            props["value"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("tree_path", "from", "to", "condition");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String tree_path = args_string(ctx.args, "tree_path");
        String from = args_string(ctx.args, "from");
        String to = args_string(ctx.args, "to");
        String condition = args_string(ctx.args, "condition");
        bool value = args_bool(ctx.args, "value", true);

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

        if (!sm->has_transition(godot::StringName(from), godot::StringName(to))) {
            return ToolResult::err("TRANSITION_NOT_FOUND",
                String("No transition from '") + from + "' to '" + to + "'");
        }

        int32_t trans_count = sm->get_transition_count();
        int32_t trans_idx = -1;
        for (int32_t i = 0; i < trans_count; i++) {
            if (sm->get_transition_from(i) == godot::StringName(from) &&
                sm->get_transition_to(i) == godot::StringName(to)) {
                trans_idx = i;
                break;
            }
        }
        if (trans_idx < 0) {
            return ToolResult::err("TRANSITION_NOT_FOUND",
                String("Transition index not found for '") + from + "' -> '" + to + "'");
        }

        godot::Ref<godot::AnimationNodeStateMachineTransition> trans = sm->get_transition(trans_idx);
        if (trans.is_null()) {
            return ToolResult::err("TRANSITION_NULL", "Transition reference is null");
        }

        godot::StringName old_condition = trans->get_advance_condition();
        godot::StringName new_condition = value ? godot::StringName(condition) : godot::StringName();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            trans->set_advance_condition(new_condition);
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Set Transition Condition ") + from + " -> " + to,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(trans.ptr(), "set_advance_condition", new_condition);
            ur->add_undo_method(trans.ptr(), "set_advance_condition", old_condition);
            ur->commit_action();
        }

        Dictionary data;
        data["from"] = from;
        data["to"] = to;
        data["condition"] = condition;
        data["value"] = value;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
