#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot_mcp {

class SetKeyframeTool : public ITool {
public:
    String name() const noexcept override { return "set_keyframe"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Insert, modify, or delete a keyframe on a track";
    }
    String description() const override {
        return "Operations on individual keyframes: insert (add a new key), "
               "delete (remove a key at the given time), or set_value (modify "
               "an existing key's value). All mutations use EditorUndoRedoManager.";
    }
    Dictionary build_input_schema() const override {
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
            p["type"] = "integer";
            p["description"] = "Track index";
            props["track_index"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Time position in seconds";
            props["time"] = p;
        }
        {
            Dictionary p;
            p["description"] = "Key value (type depends on track)";
            props["value"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Operation: insert, delete, or set_value";
            p["default"] = "insert";
            props["operation"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("anim_player_path", "clip_name", "track_index", "time");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String anim_player_path = args_string(ctx.args, "anim_player_path");
        String library_name = args_string(ctx.args, "library_name", "");
        String clip_name = args_string(ctx.args, "clip_name");
        int64_t track_index = args_int(ctx.args, "track_index");
        double time = args_float(ctx.args, "time");
        Variant value = ctx.args.has("value") ? ctx.args["value"] : Variant();
        String operation = args_string(ctx.args, "operation", "insert");

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, anim_player_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        auto *player = Object::cast_to<godot::AnimationPlayer>(node);
        if (!player) {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an AnimationPlayer: ") + anim_player_path);
        }

        godot::Ref<godot::AnimationLibrary> library;
        if (library_name.is_empty()) {
            godot::TypedArray<godot::StringName> libs = player->get_animation_library_list();
            if (libs.size() == 0) {
                return ToolResult::err("NO_LIBRARY", "No AnimationLibraries found");
            }
            library_name = String(libs[0]);
            library = player->get_animation_library(godot::StringName(library_name));
        } else {
            library = player->get_animation_library(godot::StringName(library_name));
        }
        if (library.is_null()) {
            return ToolResult::err("LIBRARY_NOT_FOUND",
                String("Library not found: ") + library_name);
        }

        godot::Ref<godot::Animation> animation = library->get_animation(godot::StringName(clip_name));
        if (animation.is_null()) {
            return ToolResult::err("CLIP_NOT_FOUND",
                String("Animation clip not found: ") + clip_name);
        }

        if (track_index < 0 || track_index >= animation->get_track_count()) {
            return ToolResult::err("INVALID_TRACK",
                String("Track index out of range: ") +
                String::num_int64(track_index));
        }

        // Normalize value: parse string representations and convert for animation tracks
        if (value.get_type() == Variant::STRING) {
            Variant parsed = godot::UtilityFunctions::str_to_var(value);
            if (parsed.get_type() != Variant::NIL) {
                value = parsed;
            }
        }
        {
            godot::Animation::TrackType ttype = animation->track_get_type(static_cast<int32_t>(track_index));
            if ((ttype == godot::Animation::TYPE_POSITION_3D || ttype == godot::Animation::TYPE_ROTATION_3D || ttype == godot::Animation::TYPE_SCALE_3D) &&
                value.get_type() == Variant::VECTOR2) {
                godot::Vector2 v2 = value;
                value = godot::Vector3(v2.x, v2.y, 0.0);
            }
        }

        auto *ur = get_undo_redo();

        if (operation == "insert") {
            if (value.get_type() == Variant::NIL) {
                return ToolResult::err("MISSING_ARG", "value is required for insert operation");
            }
            // Detect a pre-existing key at this time. The old undo just did
            // track_remove_key_at_time, which would also delete a keyframe that
            // existed BEFORE this insert �?silently destroying data on Ctrl+Z.
            // If a key already exists, we promote this to a set_value (capture
            // and restore the original); otherwise the insert undo is safe.
            const int32_t existing_idx = animation->track_find_key(
                static_cast<int32_t>(track_index), time, godot::Animation::FIND_MODE_APPROX);
            const bool had_key = existing_idx >= 0;
            const double existing_time = had_key
                ? animation->track_get_key_time(static_cast<int32_t>(track_index), existing_idx)
                : 0.0;
            const Variant existing_value = had_key
                ? animation->track_get_key_value(static_cast<int32_t>(track_index), existing_idx)
                : Variant();

            if (!ur) {
                animation->track_insert_key(static_cast<int32_t>(track_index), time, value);
                mark_scene_dirty();
            } else {
                auto *ur_ins = begin_undo_action("MCP: Insert Keyframe", ctx.root);
                if (!ur_ins) {
                    animation->track_insert_key(static_cast<int32_t>(track_index), time, value);
                    mark_scene_dirty();
                } else {
                    ur_ins->add_do_method(animation.ptr(), "track_insert_key",
                                      static_cast<int32_t>(track_index), time, value);
                    if (had_key) {
                        ur_ins->add_undo_method(animation.ptr(), "track_insert_key",
                                            static_cast<int32_t>(track_index), existing_time, existing_value);
                    } else {
                        ur_ins->add_undo_method(animation.ptr(), "track_remove_key_at_time",
                                            static_cast<int32_t>(track_index), time);
                    }
                    commit_undo_action(ur_ins);
                }
            }
        } else if (operation == "delete") {
            int32_t key_idx = animation->track_find_key(
                static_cast<int32_t>(track_index), time, godot::Animation::FIND_MODE_APPROX);
            if (key_idx < 0) {
                return ToolResult::err("KEY_NOT_FOUND",
                    String("No key found at time: ") + String::num(time));
            }
            double exact_time = animation->track_get_key_time(static_cast<int32_t>(track_index), key_idx);
            Variant old_value = animation->track_get_key_value(static_cast<int32_t>(track_index), key_idx);

            if (!ur) {
                animation->track_remove_key_at_time(static_cast<int32_t>(track_index), time);
                mark_scene_dirty();
            } else {
                auto *ur_del = begin_undo_action("MCP: Delete Keyframe", ctx.root);
                if (!ur_del) {
                    animation->track_remove_key_at_time(static_cast<int32_t>(track_index), time);
                    mark_scene_dirty();
                } else {
                    ur_del->add_do_method(animation.ptr(), "track_remove_key_at_time",
                                      static_cast<int32_t>(track_index), time);
                    ur_del->add_undo_method(animation.ptr(), "track_insert_key",
                                        static_cast<int32_t>(track_index), exact_time, old_value);
                    commit_undo_action(ur_del);
                }
            }
        } else if (operation == "set_value") {
            if (value.get_type() == Variant::NIL) {
                return ToolResult::err("MISSING_ARG", "value is required for set_value operation");
            }
            int32_t key_idx = animation->track_find_key(
                static_cast<int32_t>(track_index), time, godot::Animation::FIND_MODE_APPROX);
            if (key_idx < 0) {
                return ToolResult::err("KEY_NOT_FOUND",
                    String("No key found at time: ") + String::num(time));
            }
            Variant old_value = animation->track_get_key_value(static_cast<int32_t>(track_index), key_idx);

            if (!ur) {
                animation->track_set_key_value(static_cast<int32_t>(track_index), key_idx, value);
                mark_scene_dirty();
            } else {
                auto *ur_set = begin_undo_action("MCP: Set Keyframe Value", ctx.root);
                if (!ur_set) {
                    animation->track_set_key_value(static_cast<int32_t>(track_index), key_idx, value);
                    mark_scene_dirty();
                } else {
                    ur_set->add_do_method(animation.ptr(), "track_set_key_value",
                                      static_cast<int32_t>(track_index), key_idx, value);
                    ur_set->add_undo_method(animation.ptr(), "track_set_key_value",
                                        static_cast<int32_t>(track_index), key_idx, old_value);
                    commit_undo_action(ur_set);
                }
            }
        } else {
            return ToolResult::err("INVALID_OPERATION",
                String("Unknown operation: ") + operation +
                " (expected insert/delete/set_value)");
        }

        int32_t key_count = animation->track_get_key_count(static_cast<int32_t>(track_index));

        Dictionary data;
        data["operation"] = operation;
        data["track_index"] = static_cast<int64_t>(track_index);
        data["time"] = time;
        data["key_count"] = static_cast<int64_t>(key_count);
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
