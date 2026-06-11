
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>

namespace godot_mcp {

class GetAnimationInfoTool : public ITool {
public:
    String name() const override { return "get_animation_info"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Query animation data on an AnimationPlayer";
    }
    String description() const override {
        return "Read-only query returning full animation info for a given "
               "AnimationPlayer: libraries, animations, tracks, and player "
               "state (current_animation, is_playing, speed_scale). "
               "If anim_player_path is empty, resolves the first "
               "AnimationPlayer found in the scene.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the AnimationPlayer node (empty = auto-find first)";
            props["anim_player_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make();
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String anim_player_path = args_string(ctx.args, "anim_player_path", "");

        AnimationPlayer *player = nullptr;

        if (anim_player_path.is_empty()) {
            // Auto-find first AnimationPlayer in the scene
            player = _find_first_animation_player(ctx.root);
        } else {
            Node *node = resolve_node(ctx.root, anim_player_path);
            if (!node) {
                return ToolResult::err("NODE_NOT_FOUND",
                    String("AnimationPlayer not found: ") + anim_player_path);
            }
            player = Object::cast_to<AnimationPlayer>(node);
        }

        if (!player) {
            return ToolResult::err("NOT_FOUND",
                "No AnimationPlayer found in the scene");
        }

        Dictionary data;

        // Player state
        data["current_animation"] = String(player->get_current_animation());
        data["is_playing"] = player->is_playing();
        data["speed_scale"] = player->get_speed_scale();

        // Libraries
        Array libraries_arr;
        TypedArray<StringName> lib_names = player->get_animation_library_list();

        for (int64_t i = 0; i < lib_names.size(); i++) {
            StringName lib_name = lib_names[i];
            Ref<AnimationLibrary> lib = player->get_animation_library(lib_name);
            if (lib.is_null()) continue;

            Dictionary lib_dict;
            lib_dict["name"] = String(lib_name);

            TypedArray<StringName> anim_names = lib->get_animation_list();
            lib_dict["animation_count"] = (int64_t)anim_names.size();

            Array anims_arr;
            for (int64_t j = 0; j < anim_names.size(); j++) {
                StringName anim_name = anim_names[j];
                Ref<Animation> anim = lib->get_animation(anim_name);
                if (anim.is_null()) continue;

                Dictionary anim_dict;
                anim_dict["name"] = String(anim_name);
                anim_dict["length"] = (double)anim->get_length();

                int32_t loop = (int32_t)anim->get_loop_mode();
                String loop_str;
                if (loop == Animation::LOOP_NONE) loop_str = "none";
                else if (loop == Animation::LOOP_LINEAR) loop_str = "linear";
                else if (loop == Animation::LOOP_PINGPONG) loop_str = "pingpong";
                else loop_str = String::num_int64(loop);
                anim_dict["loop_mode"] = loop_str;

                // Tracks
                int32_t track_count = anim->get_track_count();
                Array tracks_arr;
                for (int32_t k = 0; k < track_count; k++) {
                    Dictionary track_dict;
                    track_dict["index"] = (int64_t)k;
                    track_dict["type"] = String::num_int64((int64_t)anim->track_get_type(k));
                    track_dict["path"] = String(anim->track_get_path(k));
                    track_dict["key_count"] = (int64_t)anim->track_get_key_count(k);
                    tracks_arr.append(track_dict);
                }
                anim_dict["tracks"] = tracks_arr;

                anims_arr.append(anim_dict);
            }
            lib_dict["animations"] = anims_arr;
            libraries_arr.append(lib_dict);
        }

        data["libraries"] = libraries_arr;

        return ToolResult::ok(data);
    }

private:
    static AnimationPlayer *_find_first_animation_player(Node *root) {
        if (!root) return nullptr;
        if (Object::cast_to<AnimationPlayer>(root)) {
            return Object::cast_to<AnimationPlayer>(root);
        }
        for (int64_t i = 0; i < root->get_child_count(); i++) {
            Node *child = root->get_child(i);
            if (!child) continue;
            AnimationPlayer *found = _find_first_animation_player(child);
            if (found) return found;
        }
        return nullptr;
    }
};

} // namespace godot_mcp

