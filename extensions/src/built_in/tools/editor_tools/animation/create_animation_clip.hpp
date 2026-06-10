// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class CreateAnimationClipTool : public ITool {
public:
    String name() const override { return "create_animation_clip"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Create an Animation resource in an existing AnimationPlayer library";
    }
    String description() const override {
        return "Creates a new Animation resource in the specified AnimationLibrary "
               "of an existing AnimationPlayer. If library_name is empty, auto-finds "
               "the first available library. Changes are undoable.";
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
            p["description"] = "Library name (empty = auto-find first library)";
            props["library_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Name for the new animation clip";
            props["clip_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Animation length in seconds";
            p["default"] = 1.0;
            props["length"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("anim_player_path", "clip_name");
        return s;
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

        // Resolve library: auto-find first if empty
        Ref<AnimationLibrary> library;
        if (library_name.is_empty()) {
            TypedArray<StringName> libs = player->get_animation_library_list();
            if (libs.size() == 0) {
                return ToolResult::err("NO_LIBRARY",
                    "AnimationPlayer has no AnimationLibraries; create one first");
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

        if (library->has_animation(StringName(clip_name))) {
            return ToolResult::err("CLIP_EXISTS",
                String::utf8("Animation clip already exists: ") + clip_name);
        }

        Ref<Animation> animation;
        animation.instantiate();
        if (animation.is_null()) {
            return ToolResult::err("CREATE_FAILED", "Failed to create Animation resource");
        }

        animation->set_length((float)length);

        EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            library->add_animation(StringName(clip_name), animation);
            mark_scene_dirty();
        } else {
            ur->create_action(String::utf8("MCP: Create Animation Clip"),
                              UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(library.ptr(), "add_animation",
                              StringName(clip_name), animation);
            ur->add_undo_method(library.ptr(), "remove_animation",
                                StringName(clip_name));
            ur->commit_action();
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
