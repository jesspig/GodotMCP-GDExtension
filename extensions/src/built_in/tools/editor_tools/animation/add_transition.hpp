#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_state_machine_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class AddTransitionTool : public ITool {
public:
    String name() const override { return "add_transition"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Add a transition between two states in an AnimationTree state machine";
    }
    String description() const override {
        return "Adds a transition from one state to another in the root AnimationNodeStateMachine. "
               "Configures crossfade time and switch mode. "
               "Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary build_input_schema() const override {
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
            p["type"] = "number";
            p["description"] = "Crossfade time in seconds";
            p["default"] = 0.0;
            props["xfade_time"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Switch mode: immediate/sync/at_end";
            p["default"] = "immediate";
            props["switch_mode"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("tree_path", "from", "to");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String tree_path = args_string(ctx.args, "tree_path");
        String from = args_string(ctx.args, "from");
        String to = args_string(ctx.args, "to");
        double xfade_time = args_float(ctx.args, "xfade_time", 0.0);
        String switch_mode_str = args_string(ctx.args, "switch_mode", "immediate");

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

        if (!sm->has_node(godot::StringName(from))) {
            return ToolResult::err("STATE_NOT_FOUND",
                String("Source state not found: ") + from);
        }
        if (!sm->has_node(godot::StringName(to))) {
            return ToolResult::err("STATE_NOT_FOUND",
                String("Target state not found: ") + to);
        }

        godot::AnimationNodeStateMachineTransition::SwitchMode switch_mode =
            godot::AnimationNodeStateMachineTransition::SWITCH_MODE_IMMEDIATE;
        if (switch_mode_str == "sync")
            switch_mode = godot::AnimationNodeStateMachineTransition::SWITCH_MODE_SYNC;
        else if (switch_mode_str == "at_end")
            switch_mode = godot::AnimationNodeStateMachineTransition::SWITCH_MODE_AT_END;
        else if (switch_mode_str != "immediate") {
            return ToolResult::err("INVALID_SWITCH_MODE",
                String("Unknown switch_mode: ") + switch_mode_str);
        }

        godot::Ref<godot::AnimationNodeStateMachineTransition> trans;
        trans.instantiate();
        if (trans.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create AnimationNodeStateMachineTransition");
        }
        trans->set_xfade_time(static_cast<float>(xfade_time));
        trans->set_switch_mode(switch_mode);

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            sm->add_transition(godot::StringName(from), godot::StringName(to), trans);
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Add Transition ") + from + " -> " + to,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(sm.ptr(), "add_transition",
                              godot::StringName(from), godot::StringName(to), trans);
            ur->add_undo_method(sm.ptr(), "remove_transition",
                                godot::StringName(from), godot::StringName(to));
            ur->commit_action();
        }

        Dictionary data;
        data["from"] = from;
        data["to"] = to;
        data["xfade_time"] = xfade_time;
        data["switch_mode"] = switch_mode_str;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
