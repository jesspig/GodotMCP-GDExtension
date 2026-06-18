
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateAnimationPlayerTool : public ITool {
public:
    String name() const noexcept override { return "create_animation_player"; }
    String category() const noexcept override { return "editor_tools/animation"; }
    String brief() const noexcept override {
        return "Create an AnimationPlayer node in the scene";
    }
    String description() const override {
        return "Creates an AnimationPlayer node as a child of the specified parent path. "
               "Optionally creates an AnimationLibrary and adds it to the player. "
               "All changes are committed via EditorUndoRedoManager.";
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("parent_path", "string", "Parent node path (empty = scene root)")
            .prop("node_name", "string", "Node name for the AnimationPlayer", "AnimationPlayer")
            .prop("library_name", "string", "Optional AnimationLibrary name to create and add")
            .build();
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path", "");
        String node_name = args_string(ctx.args, "node_name", "AnimationPlayer");
        String library_name = args_string(ctx.args, "library_name", "");

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        auto *player = Object::cast_to<godot::AnimationPlayer>(ClassDB::instantiate("AnimationPlayer"));
        if (!player) {
            return ToolResult::err("CREATE_FAILED", "Failed to create AnimationPlayer node");
        }
        MemdeleteGuard<godot::AnimationPlayer> guard(player);
        player->set_name(node_name);

        bool library_created = false;
        godot::Ref<godot::AnimationLibrary> lib;

        if (!library_name.is_empty()) {
            lib.instantiate();
            if (lib.is_null()) {
                return ToolResult::err("CREATE_FAILED", "Failed to create AnimationLibrary");
            }
            library_created = true;
        }

        auto *ur = begin_undo_action("MCP: Create AnimationPlayer");
        if (!ur) {
            parent->add_child(player, true, Node::INTERNAL_MODE_DISABLED);
            player->set_owner(ctx.root);
            if (library_created) {
                player->add_animation_library(godot::StringName(library_name), lib);
            }
            mark_scene_dirty();
        } else {
            ur->add_do_method(parent, "add_child", player, true,
                              static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_undo_method(parent, "remove_child", player);
            ur->add_do_method(player, "set_owner", ctx.root);
            ur->add_do_reference(player);
            ur->add_undo_reference(player);

            if (library_created) {
                ur->add_do_method(player, "add_animation_library",
                                  godot::StringName(library_name), lib);
                ur->add_undo_method(player, "remove_animation_library",
                                    godot::StringName(library_name));
            }

            commit_undo_action(ur);
        }
        guard.dismiss();

        Dictionary data;
        data["node_name"] = node_name;
        data["node_path"] = relative_path(ctx.root, player);
        data["library_created"] = library_created;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

