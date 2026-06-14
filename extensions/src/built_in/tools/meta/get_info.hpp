
#pragma once

#include "built_in/tool_base.hpp"
#include "runtime/bridge.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class GetInfoTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "get_info"; }
    String category() const override { return "meta_tools"; }
    String brief() const override { return String("Return connection status and engine/plugin/project/editor information"); }
    String description() const override {
        return String("Returns the editor's runtime information, including connection status, "
                      "engine version, project configuration, and editor state (current scene, "
                      "play status, open scenes list).");
    }
    Dictionary input_schema() const override { Dictionary s; s["type"] = "object"; s["properties"] = Dictionary(); return s; }
    bool is_meta() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &) override {
        Dictionary result;

        Dictionary conn;
        conn["status"] = "ok";
        result["connection"] = conn;

        Dictionary engine;
        Dictionary vi = godot::Engine::get_singleton()->get_version_info();
        engine["version"] = vi.get("string", String());
        engine["hash"] = vi.get("hash", String());
        result["engine"] = engine;

        Dictionary plugin;
        if (reg_) {
            plugin["builtin_tools"] = reg_->builtin_tool_count();
            plugin["custom_tools"] = reg_->custom_tool_count();
            plugin["version"] = reg_->plugin_version();
        }
        result["plugin"] = plugin;

        Dictionary project;
        godot::ProjectSettings *ps = godot::ProjectSettings::get_singleton();
        project["name"] = ps->get_setting("application/config/name", String());
        project["path"] = ps->globalize_path("res://");
        Variant main_scene = ps->get_setting("application/run/main_scene");
        project["main_scene"] = main_scene.get_type() != Variant::NIL ? String(main_scene) : String();
        result["project"] = project;

        Dictionary editor;
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            String current = ei->get_edited_scene_root()
                                 ? ei->get_edited_scene_root()->get_scene_file_path()
                                 : String();
            editor["current_scene"] = current;
            editor["is_playing"] = ei->is_playing_scene();
            PackedStringArray open_scenes = ei->get_open_scenes();
            Array open_arr;
            for (int i = 0; i < open_scenes.size(); ++i) {
                open_arr.push_back(open_scenes[i]);
            }
            editor["open_scenes"] = open_arr;
        }
        editor["error_count"] = 0;
        result["editor"] = editor;

        Dictionary bridge;
        if (reg_) {
            RuntimeBridge *rb = reg_->get_runtime_bridge();
            if (rb) {
                bridge["status"] = static_cast<int>(rb->status());
                bridge["port"] = rb->port();
                bridge["connected"] = rb->is_connected();
            }
        }
        result["bridge"] = bridge;

        return ToolResult::ok(result);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
