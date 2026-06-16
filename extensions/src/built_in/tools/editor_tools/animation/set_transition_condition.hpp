#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_state_machine_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class SetTransitionConditionTool : public ITool {
public:
    String name() const noexcept override { return "set_transition_condition"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Set a condition on a state machine transition";
    }
    String description() const override {
        return "Sets or clears a condition name on a transition between two states "
               "in the root AnimationNodeStateMachine. The condition controls when "
               "the transition fires. Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("tree_path", "string", "Path to the AnimationTree node")
            .prop("from", "string", "Source state name")
            .prop("to", "string", "Target state name")
            .prop("condition", "string", "Condition name to set on the transition")
            .prop("value", "boolean", "Whether to set or clear the condition (default: true)", true)
            .required({"tree_path", "from", "to", "condition"})
            .build();
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

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, tree_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        auto *tree = Object::cast_to<godot::AnimationTree>(node);
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

        auto *ur = begin_undo_action("MCP: Set Transition Condition " + from + " -> " + to);
        if (!ur) {
            trans->set_advance_condition(new_condition);
            mark_scene_dirty();
        } else {
            ur->add_do_method(trans.ptr(), "set_advance_condition", new_condition);
            ur->add_undo_method(trans.ptr(), "set_advance_condition", old_condition);
            commit_undo_action(ur);
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
