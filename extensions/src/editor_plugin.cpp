#include "editor_plugin.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tool_base.hpp"
#include "logging.hpp"
#include "sdk/mcp_tool_registry.hpp"
#include "ui/mcp_console.hpp"


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

std::string g_mcp_self_plugin_path = "res://addons/godot_mcp";

McpEditorPlugin::McpEditorPlugin() = default;

McpEditorPlugin::~McpEditorPlugin() = default;

void McpEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_confirm_allow", "id"), &McpEditorPlugin::_on_confirm_allow);
    ClassDB::bind_method(D_METHOD("_on_confirm_deny", "id"), &McpEditorPlugin::_on_confirm_deny);
    ClassDB::bind_method(D_METHOD("_on_confirm_allow_all", "id"), &McpEditorPlugin::_on_confirm_allow_all);
}

void McpEditorPlugin::_on_confirm_allow(const Variant &id) {
    mcp_handler_.resolve_pending_op(id, true, false);
}

void McpEditorPlugin::_on_confirm_deny(const Variant &id) {
    mcp_handler_.resolve_pending_op(id, false, false);
}

void McpEditorPlugin::_on_confirm_allow_all(const Variant &id) {
    mcp_handler_.resolve_pending_op(id, true, true);
}

String McpEditorPlugin::_get_plugin_name() const {
    return "Godot MCP";
}

int McpEditorPlugin::read_port_from_env(const String &env_var, int default_port) {
    return godot_mcp::read_port_from_env(env_var, default_port);
}

void McpEditorPlugin::load_config() {
    ProjectSettings *ps = ProjectSettings::get_singleton();

    register_project_settings();

    Variant http_port_v = ps->get_setting("godot_mcp/http_port");
    if (http_port_v.get_type() == Variant::INT) {
        http_port_ = static_cast<int>(static_cast<int64_t>(http_port_v));
    } else {
        http_port_ = read_port_from_env("GODOT_MCP_HTTP_PORT", 9600);
    }

    Variant http_host_v = ps->get_setting("godot_mcp/http_host");
    if (http_host_v.get_type() == Variant::STRING) {
        http_host_ = static_cast<String>(http_host_v);
    } else {
        http_host_ = OS::get_singleton()->get_environment("GODOT_MCP_HTTP_HOST");
        if (http_host_.is_empty()) http_host_ = "127.0.0.1";
    }

    Variant bridge_port_v = ps->get_setting("godot_mcp/bridge_port");
    if (bridge_port_v.get_type() == Variant::INT) {
        bridge_port_ = static_cast<int>(static_cast<int64_t>(bridge_port_v));
    } else {
        bridge_port_ = read_port_from_env("GODOT_MCP_BRIDGE_PORT", 9601);
    }

    Variant log_dir_v = ps->get_setting("godot_mcp/log_dir");
    if (log_dir_v.get_type() == Variant::STRING) {
        logger_.set_log_dir(static_cast<String>(log_dir_v));
    }

    Variant max_log_v = ps->get_setting("godot_mcp/max_log_entries");
    if (max_log_v.get_type() == Variant::INT) {
        logger_.set_max_entries(static_cast<int>(static_cast<int64_t>(max_log_v)));
    }
}

void McpEditorPlugin::register_project_settings() {
    ProjectSettings *ps = ProjectSettings::get_singleton();

    // http_port: basic, restart required
    if (!ps->has_setting("godot_mcp/http_port")) {
        ps->set_setting("godot_mcp/http_port", 9600);
    }
    ps->set_initial_value("godot_mcp/http_port", 9600);
    ps->set_as_basic("godot_mcp/http_port", true);
    ps->set_restart_if_changed("godot_mcp/http_port", true);

    // http_host: basic, restart required
    if (!ps->has_setting("godot_mcp/http_host")) {
        ps->set_setting("godot_mcp/http_host", "127.0.0.1");
    }
    ps->set_initial_value("godot_mcp/http_host", "127.0.0.1");
    ps->set_as_basic("godot_mcp/http_host", true);
    ps->set_restart_if_changed("godot_mcp/http_host", true);

    // bridge_port: basic, restart required
    if (!ps->has_setting("godot_mcp/bridge_port")) {
        ps->set_setting("godot_mcp/bridge_port", 9601);
    }
    ps->set_initial_value("godot_mcp/bridge_port", 9601);
    ps->set_as_basic("godot_mcp/bridge_port", true);
    ps->set_restart_if_changed("godot_mcp/bridge_port", true);

    // log_dir: basic
    if (!ps->has_setting("godot_mcp/log_dir")) {
        ps->set_setting("godot_mcp/log_dir", "res://.mcp_logs");
    }
    ps->set_initial_value("godot_mcp/log_dir", "res://.mcp_logs");
    ps->set_as_basic("godot_mcp/log_dir", true);

    // max_log_entries: basic
    if (!ps->has_setting("godot_mcp/max_log_entries")) {
        ps->set_setting("godot_mcp/max_log_entries", 500);
    }
    ps->set_initial_value("godot_mcp/max_log_entries", 500);
    ps->set_as_basic("godot_mcp/max_log_entries", true);
}

void McpEditorPlugin::save_config() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    ps->set_setting("godot_mcp/http_port", http_port_);
    ps->set_setting("godot_mcp/http_host", http_host_);
    ps->set_setting("godot_mcp/bridge_port", bridge_port_);
    ps->set_setting("godot_mcp/log_dir", logger_.log_dir());
    ps->set_setting("godot_mcp/max_log_entries", logger_.max_entries());
    ps->save();
}

void McpEditorPlugin::restart_server() {
    http_server_.stop();
    runtime_bridge_.disconnect();
    load_config();

    constexpr int kMaxPortAttempts = 10;
    bool server_started = false;

    for (int i = 0; i < kMaxPortAttempts; ++i) {
        const int try_port = http_port_ + i;
        const Error err = http_server_.start(
            static_cast<uint16_t>(try_port), &mcp_handler_, http_host_);
        if (err == OK) {
            actual_http_port_ = try_port;
            actual_bridge_port_ = bridge_port_ + i;
            server_started = true;
            break;
        }
    }

    if (!server_started) {
        log_error("plugin", "Server restart failed - all ports in use");
        started_ = false;
        return;
    }

    http_server_.set_test_engine(&test_engine_);
    runtime_bridge_.set_port(actual_bridge_port_);
    started_ = true;
    log_info("plugin", String("Server restarted on HTTP :") + String::num_int64(actual_http_port_));
}

void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;
    if (started_) return;

    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", String()));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_itools(registry_);

    // Wire RuntimeBridge into HandlerRegistry (runtime tools need it)
    registry_.set_runtime_bridge(&runtime_bridge_);

    load_config();
    runtime_bridge_.set_port(bridge_port_);

    // Port fallback: try http_port_ + 0..9
    constexpr int kMaxPortAttempts = 10;
    bool server_started = false;

    for (int i = 0; i < kMaxPortAttempts; ++i) {
        const int try_port = http_port_ + i;
        const Error err = http_server_.start(
            static_cast<uint16_t>(try_port), &mcp_handler_, http_host_);
        if (err == OK) {
            actual_http_port_ = try_port;
            actual_bridge_port_ = bridge_port_ + i;
            server_started = true;
            break;
        }
        log_warn("plugin",
            String("Port ") + String::num_int64(try_port) +
            String(" in use, trying next..."));
    }

    if (!server_started) {
        log_error("plugin",
            String("Failed to start HTTP server after ") +
            String::num_int64(kMaxPortAttempts) + String(" attempts"));
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

    // Confirmation dialog for destructive operations
    confirm_dialog_ = memnew(McpConfirmDialog);
    confirm_dialog_->connect("confirmed", Callable(this, "_on_confirm_allow"));
    confirm_dialog_->connect("denied", Callable(this, "_on_confirm_deny"));
    confirm_dialog_->connect("allow_all_session", Callable(this, "_on_confirm_allow_all"));

    mcp_handler_.set_confirm_callback([this](const PendingDestructiveOp &op) {
        pending_dialog_op_ = op;
    });

    // Wire up TestEngine
    http_server_.set_test_engine(&test_engine_);

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

    // Bottom MCP Console (with integrated status bar)
    mcp_console_ = memnew(McpConsole);
    mcp_console_->set_logger(&logger_);
    mcp_console_->set_registry(&registry_);
    mcp_console_->set_plugin(this);
    logger_.set_log_callback([this](const McpLogger::LogEntry &entry) {
        if (mcp_console_) {
            mcp_console_->on_log_appended(entry);
        }
    });
    mcp_console_->refresh();
    add_control_to_bottom_panel(mcp_console_, "MCP Console");

    // Auto-detect own plugin path for self-protection checks
    Variant self_path_v = call("get_plugin_path");
    if (self_path_v.get_type() == Variant::STRING) {
        g_mcp_self_plugin_path = String(self_path_v).utf8().get_data();
    }

    started_ = true;

    log_info("plugin", String("Godot MCP v") + String(GODOT_MCP_PLUGIN_VERSION) +
                           String(" ready on HTTP :") + String::num_int64(actual_http_port_) +
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
    if (mcp_console_) {
        remove_control_from_bottom_panel(mcp_console_);
        memdelete(mcp_console_);
        mcp_console_ = nullptr;
    }

    if (confirm_dialog_) {
        confirm_dialog_->queue_free();
        confirm_dialog_ = nullptr;
    }
    mcp_handler_.set_confirm_callback(nullptr);
    mcp_handler_.reset_session_flags();

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

void McpEditorPlugin::_process(double /*delta*/) {
    if (!started_) return;

    http_server_.poll();

    // Show pending confirmation dialog (deferred from poll callback)
    if (pending_dialog_op_.has_value()) {
        confirm_dialog_->show_for_op(
            pending_dialog_op_->jsonrpc_id,
            pending_dialog_op_->tool_name,
            pending_dialog_op_->arguments);
        pending_dialog_op_.reset();
    }

    mcp_handler_.check_pending_timeouts(kConfirmTimeoutMs);

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
