// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace godot_mcp {

class GodotInfoTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const override { return "godot_info"; }
    String category() const override { return "meta"; }
    String brief() const override { return String::utf8("返回连接状态与引擎/插件/项目/编辑器信息"); }
    String description() const override {
        return String::utf8("返回 Godot 编辑器的运行时信息，包括连接状态、引擎版本、"
                            "项目配置、编辑器状态（当前场景、播放状态、打开场景列表）");
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
        Dictionary vi = Engine::get_singleton()->get_version_info();
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
        ProjectSettings *ps = ProjectSettings::get_singleton();
        project["name"] = ps->get_setting("application/config/name", String());
        project["path"] = ps->globalize_path("res://");
        Variant main_scene = ps->get_setting("application/run/main_scene");
        project["main_scene"] = main_scene.get_type() != Variant::NIL ? main_scene : String();
        result["project"] = project;

        Dictionary editor;
        EditorInterface *ei = EditorInterface::get_singleton();
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

        return ToolResult::ok(result);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
