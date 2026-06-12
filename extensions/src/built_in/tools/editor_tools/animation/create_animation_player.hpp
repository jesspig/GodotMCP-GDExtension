
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateAnimationPlayerTool : public ITool {
public:
    String name() const override { return "create_animation_player"; }
    String category() const override { return "editor_tools/animation"; }
    String brief() const override {
        return "Create an AnimationPlayer node in the scene";
    }
    String description() const override {
        return "Creates an AnimationPlayer node as a child of the specified parent path. "
               "Optionally creates an AnimationLibrary and adds it to the player. "
               "All changes are committed via EditorUndoRedoManager.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path (empty = scene root)";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name for the AnimationPlayer";
            p["default"] = "AnimationPlayer";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Optional AnimationLibrary name to create and add";
            props["library_name"] = p;
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
        String parent_path = args_string(ctx.args, "parent_path", "");
        String node_name = args_string(ctx.args, "node_name", "AnimationPlayer");
        String library_name = args_string(ctx.args, "library_name", "");

        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Parent node not found: ") + parent_path);
        }

        Node *player_node = Object::cast_to<Node>(ClassDB::instantiate("AnimationPlayer"));
        if (!player_node) {
            return ToolResult::err("CREATE_FAILED", "Failed to create AnimationPlayer node");
        }
        player_node->set_name(node_name);

        godot::AnimationPlayer *player = Object::cast_to<godot::AnimationPlayer>(player_node);

        bool library_created = false;
        godot::Ref<godot::AnimationLibrary> lib;

        if (!library_name.is_empty()) {
            lib.instantiate();
            if (lib.is_null()) {
                memdelete(player_node);
                return ToolResult::err("CREATE_FAILED", "Failed to create AnimationLibrary");
            }
            library_created = true;
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->add_child(player_node, true, Node::INTERNAL_MODE_DISABLED);
            player_node->set_owner(ctx.root);
            if (library_created) {
                player->add_animation_library(godot::StringName(library_name), lib);
            }
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Create AnimationPlayer"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "add_child", player_node, true,
                              (int64_t)Node::INTERNAL_MODE_DISABLED);
            ur->add_undo_method(parent, "remove_child", player_node);
            ur->add_do_method(player_node, "set_owner", ctx.root);
            ur->add_do_reference(player_node);
            ur->add_undo_reference(player_node);

            if (library_created) {
                ur->add_do_method(player, "add_animation_library",
                                  godot::StringName(library_name), lib);
                ur->add_undo_method(player, "remove_animation_library",
                                    godot::StringName(library_name));
            }

            ur->commit_action();
        }

        Dictionary data;
        data["node_name"] = node_name;
        data["node_path"] = relative_path(ctx.root, player_node);
        data["library_created"] = library_created;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp

