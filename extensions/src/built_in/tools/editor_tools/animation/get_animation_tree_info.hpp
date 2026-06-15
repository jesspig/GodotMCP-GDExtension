#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation_node.hpp>
#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_state_machine_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GetAnimationTreeInfoTool : public ITool {
public:
    String name() const override { return "get_animation_tree_info"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Get AnimationTree structure info (states, transitions, parameters)";
    }
    String description() const override {
        return "Returns the tree_root type, states (name + type + position), "
               "transitions (from, to, conditions), and parameter list for an AnimationTree node.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the AnimationTree node";
            props["node_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("AnimationTree not found: ") + node_path);
        }
        godot::AnimationTree *tree = Object::cast_to<godot::AnimationTree>(node);
        if (!tree) {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an AnimationTree: ") + node_path);
        }

        godot::Ref<godot::AnimationRootNode> root = tree->get_tree_root();

        String root_type;
        if (root.is_valid()) {
            root_type = root->get_class();
        }

        Array states;
        Array transitions;

        godot::Ref<godot::AnimationNodeStateMachine> sm = Object::cast_to<godot::AnimationNodeStateMachine>(root.ptr());
        if (sm.is_valid()) {
            godot::TypedArray<godot::StringName> node_list = sm->get_node_list();
            for (int i = 0; i < node_list.size(); i++) {
                godot::StringName sn = node_list[i];
                godot::Ref<godot::AnimationNode> an = sm->get_node(sn);
                godot::Vector2 pos = sm->get_node_position(sn);

                Dictionary state;
                state["name"] = String(sn);
                state["node_type"] = an.is_valid() ? an->get_class() : "";
                Dictionary pos_dict;
                pos_dict["x"] = pos.x;
                pos_dict["y"] = pos.y;
                state["position"] = pos_dict;
                states.append(state);
            }

            int32_t trans_count = sm->get_transition_count();
            for (int32_t i = 0; i < trans_count; i++) {
                godot::Ref<godot::AnimationNodeStateMachineTransition> trans = sm->get_transition(i);
                Dictionary td;
                td["from"] = String(sm->get_transition_from(i));
                td["to"] = String(sm->get_transition_to(i));
                td["xfade_time"] = trans.is_valid() ? trans->get_xfade_time() : 0.0;
                td["switch_mode"] = trans.is_valid() ? static_cast<int64_t>(trans->get_switch_mode()) : (int64_t)0;
                td["advance_condition"] = trans.is_valid() ? String(trans->get_advance_condition()) : "";
                td["advance_mode"] = trans.is_valid() ? static_cast<int64_t>(trans->get_advance_mode()) : (int64_t)0;
                transitions.append(td);
            }
        }

        godot::Array param_list;
        if (root.is_valid()) {
            param_list = root->call("_get_parameter_list");
        }

        Dictionary data;
        data["tree_root_type"] = root_type;
        data["states"] = states;
        data["state_count"] = static_cast<int64_t>(states.size());
        data["transitions"] = transitions;
        data["transition_count"] = static_cast<int64_t>(transitions.size());
        data["parameters"] = param_list;
        data["animation_player"] = String(tree->get_animation_player());
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
