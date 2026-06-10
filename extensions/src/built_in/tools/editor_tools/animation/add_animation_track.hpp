// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

static Animation::TrackType _parse_track_type(const String &s) {
    if (s == "value") return Animation::TYPE_VALUE;
    if (s == "position") return Animation::TYPE_POSITION_3D;
    if (s == "rotation") return Animation::TYPE_ROTATION_3D;
    if (s == "scale") return Animation::TYPE_SCALE_3D;
    if (s == "blend_shape") return Animation::TYPE_BLEND_SHAPE;
    if (s == "method") return Animation::TYPE_METHOD;
    if (s == "bezier") return Animation::TYPE_BEZIER;
    if (s == "audio") return Animation::TYPE_AUDIO;
    if (s == "animation") return Animation::TYPE_ANIMATION;
    return Animation::TYPE_VALUE;
}

static Animation::InterpolationType _parse_interpolation(const String &s) {
    if (s == "nearest") return Animation::INTERPOLATION_NEAREST;
    if (s == "linear") return Animation::INTERPOLATION_LINEAR;
    if (s == "cubic") return Animation::INTERPOLATION_CUBIC;
    return Animation::INTERPOLATION_LINEAR;
}

class AddAnimationTrackTool : public ITool {
public:
    String name() const override { return "add_animation_track"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Add a new track to an existing animation clip";
    }
    String description() const override {
        return "Adds a track of the specified type to an animation clip in an "
               "AnimationPlayer library. Sets the target path for the track. "
               "Supports value, position, rotation, scale, method, bezier, audio, "
               "and animation track types. Changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the AnimationPlayer node";
            props["anim_player_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Library name (empty = auto-find first)";
            props["library_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Animation clip name";
            props["clip_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Track type (value/position/rotation/scale/method/bezier/audio/animation)";
            p["default"] = "value";
            props["track_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "NodePath to the target property (e.g. \"Sprite2D/position\")";
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Interpolation mode (nearest/linear/cubic)";
            p["default"] = "linear";
            props["interpolation"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("anim_player_path", "clip_name", "target_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String anim_player_path = args_string(ctx.args, "anim_player_path");
        String library_name = args_string(ctx.args, "library_name", "");
        String clip_name = args_string(ctx.args, "clip_name");
        String track_type_str = args_string(ctx.args, "track_type", "value");
        String target_path = args_string(ctx.args, "target_path");
        String interp_str = args_string(ctx.args, "interpolation", "linear");

        if (clip_name.is_empty() || target_path.is_empty()) {
            return ToolResult::err("MISSING_ARG", "clip_name and target_path are required");
        }

        Node *node = resolve_node(ctx.root, anim_player_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String::utf8("AnimationPlayer not found: ") + anim_player_path);
        }

        AnimationPlayer *player = Object::cast_to<AnimationPlayer>(node);
        if (!player) {
            return ToolResult::err("WRONG_TYPE",
                String::utf8("Node is not an AnimationPlayer: ") + anim_player_path);
        }

        Ref<AnimationLibrary> library;
        if (library_name.is_empty()) {
            TypedArray<StringName> libs = player->get_animation_library_list();
            if (libs.size() == 0) {
                return ToolResult::err("NO_LIBRARY",
                    "AnimationPlayer has no AnimationLibraries");
            }
            library_name = String(libs[0]);
            library = player->get_animation_library(StringName(library_name));
        } else {
            library = player->get_animation_library(StringName(library_name));
        }

        if (library.is_null()) {
            return ToolResult::err("LIBRARY_NOT_FOUND",
                String::utf8("AnimationLibrary not found: ") + library_name);
        }

        Ref<Animation> animation = library->get_animation(StringName(clip_name));
        if (animation.is_null()) {
            return ToolResult::err("CLIP_NOT_FOUND",
                String::utf8("Animation clip not found: ") + clip_name);
        }

        Animation::TrackType track_type = _parse_track_type(track_type_str);
        Animation::InterpolationType interp = _parse_interpolation(interp_str);

        int32_t track_idx = animation->get_track_count();

        EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            animation->add_track(track_type);
            animation->track_set_path(track_idx, NodePath(target_path));
            animation->track_set_interpolation_type(track_idx, interp);
            mark_scene_dirty();
        } else {
            ur->create_action(String::utf8("MCP: Add Animation Track"),
                              UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(animation.ptr(), "add_track", track_type);
            ur->add_do_method(animation.ptr(), "track_set_path",
                              track_idx, NodePath(target_path));
            ur->add_do_method(animation.ptr(), "track_set_interpolation_type",
                              track_idx, interp);
            ur->add_undo_method(animation.ptr(), "remove_track", track_idx);
            ur->commit_action();
        }

        Dictionary data;
        data["track_index"] = (int64_t)track_idx;
        data["track_type"] = track_type_str;
        data["target_path"] = target_path;
        data["interpolation"] = interp_str;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
