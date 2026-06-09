#include "editor_plugin.hpp"
#include "logging.hpp"
#include "sdk/mcp_tool_registry.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <limits>
#include <godot_cpp/variant/string.hpp>

#ifndef GODOT_MCP_PLUGIN_VERSION
#define GODOT_MCP_PLUGIN_VERSION "0.0.0-dev"
#endif

using namespace godot;

namespace godot_mcp {

McpEditorPlugin::McpEditorPlugin() = default;

McpEditorPlugin::~McpEditorPlugin() = default;

void McpEditorPlugin::_bind_methods() {}

String McpEditorPlugin::_get_plugin_name() const {
    return "Godot MCP";
}

int McpEditorPlugin::read_port_from_env(const String &env_var, int default_port) {
    OS *os = OS::get_singleton();
    if (!os) return default_port;
    const String raw = os->get_environment(env_var);
    if (raw.is_empty()) return default_port;
    const int64_t parsed = raw.to_int();
    if (parsed < 1 || parsed > std::numeric_limits<uint16_t>::max()) {
        log_warn("plugin", String("Ignoring invalid ") + env_var + String("=") + raw);
        return default_port;
    }
    return (int)parsed;
}

void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;

    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", String()));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_itools(registry_);

    // Inject dependencies into McpToolRegistry singleton (SDK layer)
    McpToolRegistry *sdk_registry = McpToolRegistry::get_singleton();
    if (!sdk_registry) {
        // Create the singleton if it doesn't exist yet
        sdk_registry = memnew(McpToolRegistry);
    }
    sdk_registry->set_handler_registry(&registry_);
    sdk_registry->set_mcp_handler(&mcp_handler_);

    // Wire RuntimeBridge into HandlerRegistry (runtime tools need it)
    registry_.set_runtime_bridge(&runtime_bridge_);

    // Load bridge port from env (default 9601)
    int bridge_port = read_port_from_env("GODOT_MCP_BRIDGE_PORT", 9601);
    runtime_bridge_.set_port(bridge_port);

    // Register EditorDebuggerPlugin for runtime bridge
    _register_debugger_plugin();

    http_port_ = read_port_from_env("GODOT_MCP_HTTP_PORT", 9600);

    if (!http_server_.start(http_port_, &mcp_handler_)) {
        log_error("plugin", "Failed to start HTTP server");
        return;
    }

    // Wire up TestEngine
    http_server_.set_test_engine(&test_engine_);

    started_ = true;

    SceneTree *tree = Object::cast_to<SceneTree>(get_tree());
    if (tree) {
        tree->connect("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame));
    }

    log_info("plugin", String("Godot MCP v") + String(GODOT_MCP_PLUGIN_VERSION) +
                           String(" ready on HTTP :") + String::num_int64(http_port_) +
                           String(" (") + String::num_int64(registry_.builtin_tool_count()) +
                           String(" builtin tools)"));
}

void McpEditorPlugin::_exit_tree() {
    if (!started_) return;

    SceneTree *tree = Object::cast_to<SceneTree>(get_tree());
    if (tree && tree->is_connected("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame))) {
        tree->disconnect("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame));
    }

    // Disconnect runtime bridge
    runtime_bridge_.disconnect();

    // Unregister EditorDebuggerPlugin
    _unregister_debugger_plugin();

    http_server_.stop();
    started_ = false;
    log_info("plugin", "Godot MCP shut down");
}

void McpEditorPlugin::_on_process_frame() {
    if (!started_) return;
    http_server_.poll();

    // Runtime bridge connection lifecycle
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) return;

    bool game_running = ei->is_playing_scene();
    if (game_running) {
        // Keep retrying while game runs but bridge not connected
        // (game TCP server may not be ready on first attempt)
        if (!game_was_running_ || !runtime_bridge_.is_connected()) {
            runtime_bridge_.connect();
        }
    } else if (game_was_running_) {
        // Game just stopped — disconnect
        runtime_bridge_.disconnect();
    }
    game_was_running_ = game_running;

    // Poll bridge connection state
    runtime_bridge_.poll();
}

void McpEditorPlugin::_register_debugger_plugin() {
    String path = "res://addons/godot_mcp/mcp_debugger_plugin.gd";
    Ref<Resource> res = ResourceLoader::get_singleton()->load(path);
    if (res.is_null()) {
        log_warn("plugin", String("Failed to load debugger plugin: ") + path);
        return;
    }
    debugger_plugin_ = res;
    call("add_debugger_plugin", debugger_plugin_);
    log_info("plugin", "EditorDebuggerPlugin registered");
}

void McpEditorPlugin::_unregister_debugger_plugin() {
    if (debugger_plugin_.is_valid()) {
        call("remove_debugger_plugin", debugger_plugin_);
        debugger_plugin_.unref();
    }
}

}  // namespace godot_mcp
