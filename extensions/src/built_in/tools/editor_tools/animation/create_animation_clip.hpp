
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class CreateAnimationClipTool : public ITool {
public:
    String name() const noexcept override { return "create_animation_clip"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Create an Animation resource in an existing AnimationPlayer library";
    }
    String description() const override {
        return "Creates a new Animation resource in the specified AnimationLibrary "
               "of an existing AnimationPlayer. If library_name is empty, auto-finds "
               "the first available library. Changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("anim_player_path", "string", "Path to the AnimationPlayer node")
            .prop("library_name", "string", "Library name (empty = auto-find first library)")
            .prop("clip_name", "string", "Name for the new animation clip")
            .prop("length", "number", "Animation length in seconds", 1.0)
            .required({"anim_player_path", "clip_name"})
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String anim_player_path = args_string(ctx.args, "anim_player_path");
        String library_name = args_string(ctx.args, "library_name", "");
        String clip_name = args_string(ctx.args, "clip_name");
        double length = args_float(ctx.args, "length", 1.0);

        if (clip_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", "clip_name is required");
        }

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, anim_player_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        auto *player = Object::cast_to<godot::AnimationPlayer>(node);
        if (!player) {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an AnimationPlayer: ") + anim_player_path);
        }

        // Resolve library: auto-find first if empty
        godot::Ref<godot::AnimationLibrary> library;
        if (library_name.is_empty()) {
            godot::TypedArray<godot::StringName> libs = player->get_animation_library_list();
            if (libs.size() == 0) {
                return ToolResult::err("NO_LIBRARY",
                    "AnimationPlayer has no AnimationLibraries; create one first");
            }
            library_name = String(libs[0]);
            library = player->get_animation_library(godot::StringName(library_name));
        } else {
            library = player->get_animation_library(godot::StringName(library_name));
        }

        if (library.is_null()) {
            return ToolResult::err("LIBRARY_NOT_FOUND",
                String("AnimationLibrary not found: ") + library_name);
        }

        if (library->has_animation(godot::StringName(clip_name))) {
            return ToolResult::err("CLIP_EXISTS",
                String("Animation clip already exists: ") + clip_name);
        }

        godot::Ref<godot::Animation> animation;
        animation.instantiate();
        if (animation.is_null()) {
            return ToolResult::err("CREATE_FAILED", "Failed to create Animation resource");
        }

        animation->set_length(static_cast<float>(length));

        auto *ur = begin_undo_action("MCP: Create Animation Clip", ctx.root);
        if (!ur) {
            library->add_animation(godot::StringName(clip_name), animation);
            mark_scene_dirty();
        } else {
            ur->add_do_method(library.ptr(), "add_animation",
                              godot::StringName(clip_name), animation);
            ur->add_undo_method(library.ptr(), "remove_animation",
                                godot::StringName(clip_name));
            commit_undo_action(ur);
        }

        Dictionary data;
        data["clip_name"] = clip_name;
        data["library_name"] = library_name;
        data["track_count"] = (int64_t)0;
        data["length"] = length;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

