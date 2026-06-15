#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class SetAudioStreamTool : public ITool {
public:
    String name() const override { return "set_audio_stream"; }
    String category() const override { return "editor_tools/audio"; }
    String brief() const override {
        return "Set the audio stream resource on an audio player node";
    }
    String description() const override {
        return "Loads an audio resource from a res:// path and assigns it to an "
               "AudioStreamPlayer, AudioStreamPlayer2D, or AudioStreamPlayer3D node. "
               "Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Path to the audio player node";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "res:// path to the audio file";
            props["stream_path"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "stream_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String stream_path = args_string(ctx.args, "stream_path");

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Audio player node not found: ") + node_path);
        }

        String class_name = node->get_class();
        if (class_name != "AudioStreamPlayer" &&
            class_name != "AudioStreamPlayer2D" &&
            class_name != "AudioStreamPlayer3D") {
            return ToolResult::err("WRONG_TYPE",
                String("Node is not an audio player: ") + class_name);
        }

        godot::Ref<godot::AudioStream> stream =
            godot::ResourceLoader::get_singleton()->load(stream_path);
        if (stream.is_null()) {
            return ToolResult::err("LOAD_FAILED",
                String("Failed to load audio stream: ") + stream_path);
        }

        godot::Ref<godot::AudioStream> old_stream;
        if (class_name == "AudioStreamPlayer") {
            godot::AudioStreamPlayer *p = Object::cast_to<godot::AudioStreamPlayer>(node);
            old_stream = p->get_stream();
        } else if (class_name == "AudioStreamPlayer2D") {
            godot::AudioStreamPlayer2D *p = Object::cast_to<godot::AudioStreamPlayer2D>(node);
            old_stream = p->get_stream();
        } else {
            godot::AudioStreamPlayer3D *p = Object::cast_to<godot::AudioStreamPlayer3D>(node);
            old_stream = p->get_stream();
        }

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            if (class_name == "AudioStreamPlayer") {
                Object::cast_to<godot::AudioStreamPlayer>(node)->set_stream(stream);
            } else if (class_name == "AudioStreamPlayer2D") {
                Object::cast_to<godot::AudioStreamPlayer2D>(node)->set_stream(stream);
            } else {
                Object::cast_to<godot::AudioStreamPlayer3D>(node)->set_stream(stream);
            }
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Set Audio Stream"),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            if (class_name == "AudioStreamPlayer") {
                ur->add_do_method(node, "set_stream", stream);
                ur->add_undo_method(node, "set_stream", old_stream);
            } else if (class_name == "AudioStreamPlayer2D") {
                ur->add_do_method(node, "set_stream", stream);
                ur->add_undo_method(node, "set_stream", old_stream);
            } else {
                ur->add_do_method(node, "set_stream", stream);
                ur->add_undo_method(node, "set_stream", old_stream);
            }
            ur->commit_action();
        }

        Dictionary data;
        data["node_path"] = node_path;
        data["stream_path"] = stream_path;
        data["player_type"] = class_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
