
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "client_config_registry.hpp"
#include "runtime/bridge_server.hpp"
#include "server/registry/handler_registry.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

class GetInfoTool : public ITool {
public:
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }

    String name() const noexcept override { return "get_info"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Return connection status and engine/plugin/project/editor information"); }
    String description() const override {
        return String("Returns the editor's runtime information, including connection status, "
                      "engine version, project configuration, and editor state (current scene, "
                      "play status, open scenes list). Set include_configs=true to also receive "
                      "client configuration snippets.");
    }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("include_configs", "boolean", "If true, include client configuration snippets", false)
            .build();
    }

    bool is_meta() const noexcept override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
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
        auto *ps = godot::ProjectSettings::get_singleton();
        project["name"] = ps->get_setting("application/config/name", String());
        project["path"] = ps->globalize_path("res://");
        Variant main_scene = ps->get_setting("application/run/main_scene");
        project["main_scene"] = main_scene.get_type() != Variant::NIL ? String(main_scene) : String();
        result["project"] = project;

        Dictionary editor;
        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            String current = ei->get_edited_scene_root()
                                 ? ei->get_edited_scene_root()->get_scene_file_path()
                                 : String();
            editor["current_scene"] = current;
            editor["is_playing"] = ei->is_playing_scene();
            PackedStringArray open_scenes = ei->get_open_scenes();
            Array open_arr;
            for (int64_t i = 0; i < open_scenes.size(); ++i) {
                open_arr.push_back(open_scenes[i]);
            }
            editor["open_scenes"] = open_arr;
        }
        {
            auto *base = ei ? ei->get_base_control() : nullptr;
            if (base) {
                Array nodes = base->find_children("*", "EditorDebuggerNode", true, false);
                if (nodes.size() > 0) {
                    auto *dbg = Object::cast_to<Node>(nodes[0]);
                    if (dbg) {
                        Object *active = dbg->call("get_current_debugger");
                        if (active) {
                            editor["error_count"] = static_cast<int64_t>(active->call("get_error_count"));
                        } else {
                            editor["error_count"] = (int64_t)0;
                        }
                    }
                }
            }
        }
        result["editor"] = editor;

        Dictionary bridge;
        if (reg_) {
            RuntimeBridgeServer *bs = reg_->get_runtime_bridge_server();
            if (bs) {
                bridge["server_status"] = static_cast<int>(bs->status());
                bridge["server_port"] = bs->port();
                bridge["server_connected"] = bs->is_connected();
                bridge["instance_count"] = bs->instance_count();
                bridge["server_listening"] = (bs->status() == RuntimeBridgeServer::ACCEPTING);
                if (bs->has_instances()) {
                    bridge["instances"] = bs->get_connected_instances();
                }
            }
        }
        result["bridge"] = bridge;

        const bool include_configs = ctx.args.get("include_configs", false);
        if (include_configs) {
            int count = 0;
            const ClientEntry *entries = get_entries(count);
            Dictionary configs;
            const String url = String("http://127.0.0.1:") + String::num_int64(9600) + String("/mcp");
            for (int i = 0; i < count; ++i) {
                configs[String(entries[i].name)] = entries[i].generator(url);
            }
            result["client_configs"] = configs;
        }

        return ToolResult::ok(result);
    }

private:
    HandlerRegistry *reg_ = nullptr;
};

} // namespace godot_mcp
