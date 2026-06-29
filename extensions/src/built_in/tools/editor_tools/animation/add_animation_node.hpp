#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/cmd_utils/dispatch_map.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation_node.hpp>
#include <godot_cpp/classes/animation_node_animation.hpp>
#include <godot_cpp/classes/animation_node_blend2.hpp>
#include <godot_cpp/classes/animation_node_blend3.hpp>
#include <godot_cpp/classes/animation_node_blend_tree.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_node_one_shot.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_time_seek.hpp>
#include <godot_cpp/classes/animation_node_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

namespace {

bool animation_exists(godot::AnimationTree *tree, const godot::String &animation_name) {
    godot::NodePath player_path = tree->get_animation_player();
    if (player_path.is_empty()) return false;
    auto *player = Object::cast_to<godot::AnimationPlayer>(tree->get_node_internal(player_path));
    if (!player) return false;
    godot::TypedArray<godot::StringName> libs = player->get_animation_library_list();
    for (int i = 0; i < libs.size(); i++) {
        godot::StringName lib_name = libs[i];
        godot::Ref<godot::AnimationLibrary> lib = player->get_animation_library(lib_name);
        if (lib.is_null()) continue;
        godot::TypedArray<godot::StringName> anims = lib->get_animation_list();
        for (int j = 0; j < anims.size(); j++) {
            godot::String anim_key = anims[j];
            godot::String full = String(lib_name) + "/" + anim_key;
            if (full == animation_name || anim_key == animation_name) {
                return true;
            }
        }
    }
    return false;
}

} // anonymous namespace

class AddAnimationNodeTool : public ITool {
public:
    String name() const noexcept override { return "add_animation_node"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Add an animation node to an AnimationTree state machine";
    }
    String description() const override {
        return "Adds an AnimationNode subclass (animation, blend2, blend3, blend_tree, "
               "one_shot, time_seek, transition) to the root AnimationNodeStateMachine "
               "of an AnimationTree. Uses EditorUndoRedoManager for undo support.";
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
            Dictionary pos_props;
            {
                Dictionary px;
                px["type"] = "number";
                px["description"] = "X position";
                px["default"] = 0;
                pos_props["x"] = px;
                Dictionary py;
                py["type"] = "number";
                py["description"] = "Y position";
                py["default"] = 0;
                pos_props["y"] = py;
            }
            p["properties"] = pos_props;
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
        if (node_name.is_empty()) node_name = args_string(ctx.args, "node_name");
        String animation_name = args_string(ctx.args, "animation_name", "");

        godot::Vector2 position;
        Dictionary pos = args_get_typed<Dictionary>(ctx.args, "position", Dictionary());
        if (!pos.is_empty()) {
            position.x = static_cast<real_t>(args_float(pos, "x", 0.0));
            position.y = static_cast<real_t>(args_float(pos, "y", 0.0));
        }

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

        if (sm->has_node(godot::StringName(node_name))) {
            return ToolResult::err("NAME_CONFLICT",
                String("State already exists: ") + node_name);
        }

        static const auto kAnimationNodeClasses = godot_mcp::make_dispatch_map<godot::String, godot::String>(
            std::pair{godot::String("animation"),  godot::String("AnimationNodeAnimation")},
            std::pair{godot::String("blend2"),     godot::String("AnimationNodeBlend2")},
            std::pair{godot::String("blend3"),     godot::String("AnimationNodeBlend3")},
            std::pair{godot::String("blend_tree"), godot::String("AnimationNodeBlendTree")},
            std::pair{godot::String("one_shot"),   godot::String("AnimationNodeOneShot")},
            std::pair{godot::String("time_seek"),  godot::String("AnimationNodeTimeSeek")},
            std::pair{godot::String("transition"), godot::String("AnimationNodeTransition")}
        );
        assert(kAnimationNodeClasses.validate() && "Duplicate key");

        const godot::String* matched = kAnimationNodeClasses.find(godot::String(node_type));
        if (!matched) {
            return ToolResult::err("INVALID_NODE_TYPE",
                String("Unknown node_type: ") + node_type);
        }
        godot::String class_name = *matched;

        godot::Ref<godot::AnimationNode> anim_node =
            Object::cast_to<godot::AnimationNode>(
                godot::ClassDB::instantiate(class_name));
        if (anim_node.is_null()) {
            return ToolResult::err("CREATE_FAILED",
                String("Failed to create ") + class_name);
        }

        if (node_type == "animation" && !animation_name.is_empty()) {
            if (!animation_exists(tree, animation_name)) {
                return ToolResult::err("ANIM_NOT_FOUND",
                    String("Animation not found in player: ") + animation_name);
            }
            auto *anim = Object::cast_to<godot::AnimationNodeAnimation>(anim_node.ptr());
            if (anim) {
                anim->set_animation(godot::StringName(animation_name));
            }
        }

        auto *ur = begin_undo_action("MCP: Add AnimationNode " + node_name, ctx.root);
        if (!ur) {
            sm->add_node(godot::StringName(node_name), anim_node, position);
            mark_scene_dirty();
        } else {
            ur->add_do_method(sm.ptr(), "add_node",
                              godot::StringName(node_name), anim_node, position);
            ur->add_undo_method(sm.ptr(), "remove_node",
                                godot::StringName(node_name));
            commit_undo_action(ur);
        }

        Dictionary result_data;
        result_data["node_name"] = node_name;
        result_data["node_type"] = node_type;
        result_data["class_name"] = class_name;
        result_data["animation_name"] = animation_name;
        return ToolResult::ok(result_data);
    }
};

} // namespace godot_mcp
