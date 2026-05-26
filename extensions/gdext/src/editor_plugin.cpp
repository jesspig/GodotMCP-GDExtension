// =====================================================================
// editor_plugin.cpp — McpEditorPlugin implementation.
// =====================================================================

#include "editor_plugin.hpp"

#include "logging.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/string.hpp>

// Plugin version baked in at build time. CMake passes this via
// target_compile_definitions so plugin.cfg, Cargo.toml (legacy), and
// the runtime stay in lockstep.
#ifndef GODOT_MCP_PLUGIN_VERSION
#define GODOT_MCP_PLUGIN_VERSION "0.0.0-dev"
#endif

using namespace godot;

namespace godot_mcp {

McpEditorPlugin::McpEditorPlugin() = default;

McpEditorPlugin::~McpEditorPlugin() = default;

void McpEditorPlugin::_bind_methods() {
    // No exposed methods/signals yet — the dispatch loop is internal.
}

String McpEditorPlugin::_get_plugin_name() const {
    return "Godot MCP";
}

int McpEditorPlugin::read_port_from_env() {
    constexpr int kDefaultPort = 9500;
    OS *os = OS::get_singleton();
    if (!os) {
        return kDefaultPort;
    }
    const String raw = os->get_environment("GODOT_MCP_PORT");
    if (raw.is_empty()) {
        return kDefaultPort;
    }
    const int64_t parsed = raw.to_int();
    if (parsed < 1 || parsed > 65535) {
        log_warn("plugin", String("Ignoring invalid GODOT_MCP_PORT=") + raw);
        return kDefaultPort;
    }
    return (int)parsed;
}

void McpEditorPlugin::_enter_tree() {
    // Don't run when the plugin is loaded by an exported game; the
    // editor-only initialisation level should already guarantee this,
    // but be defensive.
    if (!Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    // --- Populate the handler registry. -------------------------------
    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", String()));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_all_tools(registry_);
    log_info("plugin", String("Registered ") + String::num_int64(registry_.size()) +
                               String(" tool(s)"));

    // --- Start the WebSocket server. ----------------------------------
    const int port = read_port_from_env();
    if (!ws_server_.start(port, &registry_)) {
        log_error("plugin", "Failed to start WebSocket server; tools will be unavailable");
        return;
    }
    started_ = true;
    log_info("plugin", String("Godot MCP ready (v") + String(GODOT_MCP_PLUGIN_VERSION) + String(")"));
}

void McpEditorPlugin::_exit_tree() {
    if (!started_) {
        return;
    }
    ws_server_.stop();
    started_ = false;
    log_info("plugin", "Godot MCP shut down");
}

void McpEditorPlugin::_process(double /*delta*/) {
    if (!started_) {
        return;
    }
    ws_server_.poll();
}

}  // namespace godot_mcp
