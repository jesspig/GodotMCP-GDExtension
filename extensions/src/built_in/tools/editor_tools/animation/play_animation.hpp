#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation_player.hpp>

namespace godot_mcp {

class PlayAnimationTool : public ITool {
public:
    String name() const noexcept override { return "play_animation"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Play, stop, pause, or seek an animation on an AnimationPlayer";
    }
    String description() const override {
        return "Controls animation playback on an AnimationPlayer. "
               "Supports play (start/resume an animation), stop (halt playback), "
               "pause (pause at current position), seek (jump to a time position), "
               "and get_status (query current playback state). "
               "Play can specify speed_scale, blend_time, and from_end.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("anim_player_path", "string", "Path to the AnimationPlayer node")
            .prop("action", "string", "Action: play/stop/pause/seek/get_status", "play")
            .prop("clip_name", "string", "Animation clip name to play (required for play action)")
            .prop("speed_scale", "number", "Playback speed multiplier", 1.0)
            .prop("blend_time", "number", "Crossfade blend time in seconds", -1.0)
            .prop("from_end", "boolean", "Play backwards from end", false)
            .prop("time", "number", "Time position in seconds (for seek action)", 0.0)
            .prop("keep_state", "boolean", "Keep final state on stop", false)
            .required({"anim_player_path", "action"})
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String anim_player_path = args_string(ctx.args, "anim_player_path");
        String action = args_string(ctx.args, "action", "play");
        String clip_name = args_string(ctx.args, "clip_name", "");
        double speed_scale = args_float(ctx.args, "speed_scale", 1.0);
        double blend_time = args_float(ctx.args, "blend_time", -1.0);
        bool from_end = args_bool(ctx.args, "from_end", false);
        double time = args_float(ctx.args, "time", 0.0);
        bool keep_state = args_bool(ctx.args, "keep_state", false);

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, anim_player_path, node)) {
            return *err;
        }
        auto *player = Object::cast_to<godot::AnimationPlayer>(node);
        if (!player) {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an AnimationPlayer: ") + anim_player_path);
        }

        Dictionary data;
        data["action"] = action;
        data["current_animation"] = String(player->get_current_animation());
        data["is_playing"] = player->is_playing();

        if (action == "play") {
            if (clip_name.is_empty()) {
                return ToolResult::err("MISSING_ARG", "clip_name is required for play action");
            }
            if (!player->has_animation(godot::StringName(clip_name))) {
                return ToolResult::err("CLIP_NOT_FOUND",
                    String("Animation clip not found: ") + clip_name);
            }
            player->play(godot::StringName(clip_name), blend_time, static_cast<float>(speed_scale), from_end);
            data["clip_name"] = clip_name;
            data["speed_scale"] = static_cast<double>(player->get_speed_scale());
        } else if (action == "stop") {
            player->stop(keep_state);
        } else if (action == "pause") {
            player->pause();
        } else if (action == "seek") {
            player->seek(time, true, false);
            data["time"] = time;
        } else if (action == "get_status") {
            data["is_animation_active"] = player->is_animation_active();
            data["current_animation_position"] = player->get_current_animation_position();
            data["current_animation_length"] = player->get_current_animation_length();
            data["speed_scale"] = static_cast<double>(player->get_speed_scale());
            data["playing_speed"] = static_cast<double>(player->get_playing_speed());
            data["assigned_animation"] = String(player->get_assigned_animation());
        } else {
            return ToolResult::err("INVALID_ACTION",
                String("Unknown action: ") + action + " (expected play/stop/pause/seek/get_status)");
        }

        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
