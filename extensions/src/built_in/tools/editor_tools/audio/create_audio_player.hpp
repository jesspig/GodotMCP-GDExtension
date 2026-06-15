
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateAudioPlayerTool : public ITool {
public:
    String name() const noexcept override { return "create_audio_player"; }
    String category() const noexcept override { return "editor_tools/audio"; }
    String brief() const noexcept override {
        return "Create an AudioStreamPlayer, AudioStreamPlayer2D, or AudioStreamPlayer3D node";
    }
    String description() const override {
        return "Creates an audio player node as a child of the specified parent. "
               "Supports standard (AudioStreamPlayer), 2D (AudioStreamPlayer2D), "
               "and 3D (AudioStreamPlayer3D) variants. "
               "Optionally loads an audio stream resource and configures bus, autoplay, and volume. "
               "Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Player type: standard/2d/3d";
            p["default"] = "standard";
            props["player_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name (empty = use class name)";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "res:// path to audio stream resource";
            props["stream_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Audio bus name";
            p["default"] = "Master";
            props["bus"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "Auto-play on ready";
            p["default"] = false;
            props["autoplay"] = p;
        }
        {
            Dictionary p;
            p["type"] = "number";
            p["description"] = "Volume in dB";
            p["default"] = 0.0;
            props["volume_db"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("parent_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String player_type = args_string(ctx.args, "player_type", "standard");
        String node_name = args_string(ctx.args, "node_name", "");
        String stream_path = args_string(ctx.args, "stream_path", "");
        String bus = args_string(ctx.args, "bus", "Master");
        bool autoplay = args_bool(ctx.args, "autoplay", false);
        double volume_db = args_float(ctx.args, "volume_db", 0.0);

        String class_name;
        if (player_type == "standard") class_name = "AudioStreamPlayer";
        else if (player_type == "2d") class_name = "AudioStreamPlayer2D";
        else if (player_type == "3d") class_name = "AudioStreamPlayer3D";
        else {
            return ToolResult::err("INVALID_PLAYER_TYPE",
                String("Unknown player_type: ") + player_type + " (expected standard/2d/3d)");
        }

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }

        Node *player_node = Object::cast_to<Node>(ClassDB::instantiate(class_name));
        if (!player_node) {
            return ToolResult::err("CREATE_FAILED",
                String("Failed to create ") + class_name);
        }

        if (node_name.is_empty()) {
            node_name = class_name;
        }
        player_node->set_name(node_name);

        if (parent->has_node(String("./") + node_name)) {
            memdelete(player_node);
            return ToolResult::err("NAME_CONFLICT",
                String("A node with the same name already exists: ") + node_name);
        }

        godot::Ref<godot::AudioStream> stream;
        if (!stream_path.is_empty()) {
            stream = godot::ResourceLoader::get_singleton()->load(stream_path);
            if (stream.is_null()) {
                memdelete(player_node);
                return ToolResult::err("LOAD_FAILED",
                    String("Failed to load audio stream: ") + stream_path);
            }
        }

        if (player_type == "standard") {
            auto *p = Object::cast_to<godot::AudioStreamPlayer>(player_node);
            if (stream.is_valid()) p->set_stream(stream);
            p->set_bus(godot::StringName(bus));
            p->set_autoplay(autoplay);
            p->set_volume_db(static_cast<float>(volume_db));
        } else if (player_type == "2d") {
            auto *p = Object::cast_to<godot::AudioStreamPlayer2D>(player_node);
            if (stream.is_valid()) p->set_stream(stream);
            p->set_bus(godot::StringName(bus));
            p->set_autoplay(autoplay);
            p->set_volume_db(static_cast<float>(volume_db));
        } else {
            auto *p = Object::cast_to<godot::AudioStreamPlayer3D>(player_node);
            if (stream.is_valid()) p->set_stream(stream);
            p->set_bus(godot::StringName(bus));
            p->set_autoplay(autoplay);
            p->set_volume_db(static_cast<float>(volume_db));
        }

        auto *ur = begin_undo_action("MCP: Create AudioPlayer " + class_name);
        if (!ur) {
            parent->add_child(player_node, true, Node::INTERNAL_MODE_DISABLED);
            player_node->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            ur->add_do_method(parent, "add_child", player_node, true,
                              static_cast<int64_t>(Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(player_node, "set_owner", ctx.root);
            ur->add_do_reference(player_node);
            ur->add_undo_method(player_node, "set_owner", Variant());
            ur->add_undo_method(parent, "remove_child", player_node);
            commit_undo_action(ur);
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(player_node);
            }
        }

        Dictionary data;
        data["node_name"] = player_node->get_name();
        data["node_path"] = relative_path(ctx.root, player_node);
        data["player_type"] = player_type;
        data["class_name"] = class_name;
        data["stream_loaded"] = stream.is_valid();
        data["bus"] = bus;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
