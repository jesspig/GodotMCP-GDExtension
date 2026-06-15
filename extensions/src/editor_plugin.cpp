#include "editor_plugin.hpp"
#include "logging.hpp"
#include "sdk/mcp_tool_registry.hpp"
#include "ui/mcp_console.hpp"
#include "ui/mcp_dock.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
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
    return static_cast<int>(parsed);
}

void McpEditorPlugin::load_config() {
    ProjectSettings *ps = ProjectSettings::get_singleton();

    bool any_nil = false;

    Variant http_port_v = ps->get_setting("godot_mcp/http_port");
    if (http_port_v.get_type() == Variant::INT) {
        http_port_ = static_cast<int>(static_cast<int64_t>(http_port_v));
    } else {
        any_nil = true;
        http_port_ = read_port_from_env("GODOT_MCP_HTTP_PORT", 9600);
    }

    Variant http_host_v = ps->get_setting("godot_mcp/http_host");
    if (http_host_v.get_type() == Variant::STRING) {
        http_host_ = static_cast<String>(http_host_v);
    } else {
        any_nil = true;
        http_host_ = OS::get_singleton()->get_environment("GODOT_MCP_HTTP_HOST");
        if (http_host_.is_empty()) http_host_ = "127.0.0.1";
    }

    Variant bridge_port_v = ps->get_setting("godot_mcp/bridge_port");
    if (bridge_port_v.get_type() == Variant::INT) {
        bridge_port_ = static_cast<int>(static_cast<int64_t>(bridge_port_v));
    } else {
        any_nil = true;
        bridge_port_ = read_port_from_env("GODOT_MCP_BRIDGE_PORT", 9601);
    }

    if (any_nil) {
        save_config();
    }
}

void McpEditorPlugin::save_config() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    ps->set_setting("godot_mcp/http_port", http_port_);
    ps->set_setting("godot_mcp/http_host", http_host_);
    ps->set_setting("godot_mcp/bridge_port", bridge_port_);
    ps->save();
}

void McpEditorPlugin::restart_server(bool force) {
    if (!force && mcp_handler_.has_pending_requests()) {
        pending_restart_ = true;
        restart_deadline_ = Time::get_singleton()->get_ticks_msec() / 1000.0 + kRestartTimeoutSec;
        log_info("plugin", String("Waiting for ") + String::num_int64(mcp_handler_.pending_request_count()) + String(" pending requests..."));
        return;
    }

    http_server_.stop();
    runtime_bridge_.disconnect();
    load_config();
    http_server_.start(http_port_, &mcp_handler_, http_host_);
    http_server_.set_test_engine(&test_engine_);
    runtime_bridge_.set_port(bridge_port_);
    started_ = true;
    pending_restart_ = false;
    log_info("plugin", String("Server restarted on HTTP :") + String::num_int64(http_port_));
}

void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;

    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", String()));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_itools(registry_);

    // Wire RuntimeBridge into HandlerRegistry (runtime tools need it)
    registry_.set_runtime_bridge(&runtime_bridge_);

    load_config();
    runtime_bridge_.set_port(bridge_port_);

    if (http_server_.start(http_port_, &mcp_handler_, http_host_) != OK) {
        log_error("plugin", String("Failed to start HTTP server on ") + http_host_ + String(":") + String::num_int64(http_port_));
        return;
    }

    // Inject dependencies into McpToolRegistry singleton (SDK layer)
    McpToolRegistry *sdk_registry = McpToolRegistry::get_singleton();
    if (!sdk_registry) {
        sdk_registry = memnew(McpToolRegistry);
    }
    sdk_registry->set_handler_registry(&registry_);
    sdk_registry->set_mcp_handler(&mcp_handler_);

    // Register as Engine singleton so C# can access via Engine.GetSingleton("McpToolRegistry")
    Engine::get_singleton()->register_singleton("McpToolRegistry", sdk_registry);

    // Wire up TestEngine
    http_server_.set_test_engine(&test_engine_);

    // Initialize MCP Logger
    ProjectSettings *ps = ProjectSettings::get_singleton();
    Variant ps_log_dir = ps->get_setting("godot_mcp/log_dir");
    if (ps_log_dir.get_type() == Variant::STRING) {
        logger_.set_log_dir(static_cast<String>(ps_log_dir));
    }
    Variant ps_max = ps->get_setting("godot_mcp/max_log_entries");
    if (ps_max.get_type() == Variant::INT) {
        logger_.set_max_entries(static_cast<int>(static_cast<int64_t>(ps_max)));
    }
    logger_.rotate();

    // Connect log callback
    mcp_handler_.set_log_callback([this](const McpHandler::ToolCallLog &log) {
        McpLogger::LogEntry entry;
        entry.timestamp = log.timestamp;
        entry.tool_name = log.tool_name;
        entry.success = log.success;
        entry.args = log.args;
        entry.result = log.result;
        entry.duration_ms = log.duration_ms;
        logger_.append(entry);
    });

    // Right sidebar Dock
    mcp_dock_ = memnew(McpDock);
    mcp_dock_->set_plugin(this);
    mcp_dock_->set_registry(&registry_);
    mcp_dock_->set_logger(&logger_);
    add_control_to_dock(DOCK_SLOT_RIGHT_UL, mcp_dock_);

    // Bottom MCP Console
    mcp_console_ = memnew(McpConsole);
    mcp_console_->set_logger(&logger_);
    logger_.set_log_callback([this](const McpLogger::LogEntry &entry) {
        if (mcp_console_) {
            mcp_console_->on_log_appended(entry);
        }
    });
    mcp_console_->refresh();
    add_control_to_bottom_panel(mcp_console_, "MCP Console");

    started_ = true;

    log_info("plugin", String("Godot MCP v") + String(GODOT_MCP_PLUGIN_VERSION) +
                           String(" ready on HTTP :") + String::num_int64(http_port_) +
                           String(" (") + String::num_int64(registry_.builtin_tool_count()) +
                           String(" builtin tools)"));
}

void McpEditorPlugin::_exit_tree() {
    if (!started_) return;

    // Clear log callbacks to prevent dangling lambda captures
    mcp_handler_.set_log_callback(nullptr);
    logger_.set_log_callback(nullptr);

    // Disconnect SDK registry from handler to prevent dangling references
    McpToolRegistry *sdk_registry = McpToolRegistry::get_singleton();
    if (sdk_registry) {
        sdk_registry->set_handler_registry(nullptr);
        sdk_registry->set_mcp_handler(nullptr);
    }

    // Clean up UI
    if (mcp_dock_) {
        remove_control_from_docks(mcp_dock_);
        memdelete(mcp_dock_);
        mcp_dock_ = nullptr;
    }
    if (mcp_console_) {
        remove_control_from_bottom_panel(mcp_console_);
        memdelete(mcp_console_);
        mcp_console_ = nullptr;
    }

    runtime_bridge_.disconnect();

    http_server_.stop();

    // Unregister and clean up SDK registry singleton
    if (sdk_registry) {
        Engine::get_singleton()->unregister_singleton("McpToolRegistry");
        memdelete(sdk_registry);
        sdk_registry = nullptr;
    }

    started_ = false;
    log_info("plugin", "Godot MCP shut down");
}

void McpEditorPlugin::_process(double delta) {
    if (!started_) return;

    // Pending restart polling
    if (pending_restart_) {
        bool timed_out = Time::get_singleton()->get_ticks_msec() / 1000.0 > restart_deadline_;
        if (!mcp_handler_.has_pending_requests() || timed_out) {
            restart_server(true);
        }
    }

    http_server_.poll();

    _try_bridge_connect();

    runtime_bridge_.poll();
}

void McpEditorPlugin::_try_bridge_connect() {
    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) return;

    bool game_running = ei->is_playing_scene();
    if (game_running) {
        if (!game_was_running_ || !runtime_bridge_.is_connected()) {
            runtime_bridge_.connect();
        }
    } else if (game_was_running_) {
        runtime_bridge_.disconnect();
    }
    game_was_running_ = game_running;
}

}  // namespace godot_mcp
