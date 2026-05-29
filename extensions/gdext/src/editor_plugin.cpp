#include "editor_plugin.hpp"

#include "logging.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
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
    if (parsed < 1 || parsed > 65535) {
        log_warn("plugin", String("Ignoring invalid ") + env_var + String("=") + raw);
        return default_port;
    }
    return (int)parsed;
}

void McpEditorPlugin::load_tool_schemas() {
    // Try to load the schema JSON from the addon directory.
    // The JSON file is bundled with the plugin by the build system.
    static const Vector<String> kCandidatePaths = {
        "res://addons/godot_mcp/tool_schemas.json",
    };
    for (int i = 0; i < kCandidatePaths.size(); ++i) {
        Ref<FileAccess> f = FileAccess::open(kCandidatePaths[i], FileAccess::READ);
        if (f.is_valid()) {
            const String content = f->get_as_text();
            f->close();
            registry_.load_schemas_from_json(content);
            log_info("plugin", String("Loaded tool schemas from ") + kCandidatePaths[i]);
            return;
        }
    }
    log_warn("plugin", "tool_schemas.json not found — tools will have no schema info");
}

void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;

    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", String()));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_all_tools(registry_);

    // Load schemas from bundled JSON file (adds descriptions + input schemas to registry)
    load_tool_schemas();

    log_info("plugin", String("Registered ") + String::num_int64(registry_.size()) +
                           String(" tool(s) with ") +
                           String::num_int64(registry_.get_all_tools().size()) +
                           String(" schema(s)"));

    ws_port_ = read_port_from_env("GODOT_MCP_PORT", 9500);
    http_port_ = read_port_from_env("GODOT_MCP_HTTP_PORT", 9600);

    // Start WebSocket server (legacy, for Python stdio server compatibility)
    if (!ws_server_.start(ws_port_, &registry_)) {
        log_error("plugin", "Failed to start WebSocket server");
    }

    // Start HTTP Streamable MCP server
    if (!http_server_.start(http_port_, &mcp_handler_)) {
        log_error("plugin", "Failed to start HTTP server");
    }

    started_ = true;

    // Use SceneTree::process_frame so polling survives play mode.
    SceneTree *tree = Object::cast_to<SceneTree>(get_tree());
    if (tree) {
        tree->connect("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame));
    }

    log_info("plugin", String("Godot MCP ready (v") + String(GODOT_MCP_PLUGIN_VERSION) +
                           String(") — WS :") + String::num_int64(ws_port_) +
                           String(", HTTP :") + String::num_int64(http_port_));
}

void McpEditorPlugin::_exit_tree() {
    if (!started_) return;

    SceneTree *tree = Object::cast_to<SceneTree>(get_tree());
    if (tree && tree->is_connected("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame))) {
        tree->disconnect("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame));
    }

    http_server_.stop();
    ws_server_.stop();
    started_ = false;
    log_info("plugin", "Godot MCP shut down");
}

void McpEditorPlugin::_on_process_frame() {
    if (!started_) return;

    ws_server_.poll();
    http_server_.poll();
}

}  // namespace godot_mcp
