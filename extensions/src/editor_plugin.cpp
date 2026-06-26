#include "editor_plugin.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_stack.hpp"
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

    if (!ps->has_setting("godot_mcp/http_port")) {
        ps->set_setting("godot_mcp/http_port", 9600);
    }
    ps->set_initial_value("godot_mcp/http_port", 9600);
    ps->set_as_basic("godot_mcp/http_port", true);
    ps->set_restart_if_changed("godot_mcp/http_port", true);

    if (!ps->has_setting("godot_mcp/http_host")) {
        ps->set_setting("godot_mcp/http_host", "127.0.0.1");
    }
    ps->set_initial_value("godot_mcp/http_host", "127.0.0.1");
    ps->set_as_basic("godot_mcp/http_host", true);
    ps->set_restart_if_changed("godot_mcp/http_host", true);

    if (!ps->has_setting("godot_mcp/bridge_port")) {
        ps->set_setting("godot_mcp/bridge_port", 9601);
    }
    ps->set_initial_value("godot_mcp/bridge_port", 9601);
    ps->set_as_basic("godot_mcp/bridge_port", true);
    ps->set_restart_if_changed("godot_mcp/bridge_port", true);

    if (!ps->has_setting("godot_mcp/log_dir")) {
        ps->set_setting("godot_mcp/log_dir", "res://.mcp_logs");
    }
    ps->set_initial_value("godot_mcp/log_dir", "res://.mcp_logs");
    ps->set_as_basic("godot_mcp/log_dir", true);

    if (!ps->has_setting("godot_mcp/max_log_entries")) {
        ps->set_setting("godot_mcp/max_log_entries", 500);
    }
    ps->set_initial_value("godot_mcp/max_log_entries", 500);
    ps->set_as_basic("godot_mcp/max_log_entries", true);

    UndoManager::register_setting();
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
    bridge_server_.stop();
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
    bridge_server_.set_port(actual_bridge_port_);
    bridge_server_.start(actual_bridge_port_);
    started_ = true;
    log_info("plugin", String("Server restarted on HTTP :") + String::num_int64(actual_http_port_));
}

void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;
    if (started_) return;

    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", String()));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_itools(registry_);

    // Initialize global UndoManager
    g_undo_manager = new UndoManager();

    // Wire RuntimeBridgeServer into HandlerRegistry (runtime tools need it)
    registry_.set_runtime_bridge_server(&bridge_server_);

    // Wire tools-changed notification
    registry_.set_on_tools_changed([this]() {
        mcp_handler_.notify_tools_list_changed();
    });

    load_config();
    bridge_server_.set_port(bridge_port_);

    // Wire up RuntimeBridgeServer -> McpHandler
    bridge_server_.set_mcp_handler(&mcp_handler_);
    mcp_handler_.set_bridge_server(&bridge_server_);

    // Wire up RuntimeBridgeServer response callback -> McpHandler enqueue_event
    bridge_server_.set_response_callback(
        [this](const Dictionary &response) {
            Dictionary jsonrpc;
            jsonrpc["jsonrpc"] = "2.0";
            jsonrpc["id"] = response["id"];

            if (response.has("error")) {
                Variant err_val = response["error"];
                String err_msg = "Bridge command failed";
                if (err_val.get_type() == Variant::DICTIONARY) {
                    Dictionary err_dict = err_val;
                    err_msg = err_dict.get("message", err_msg);
                }
                Dictionary err;
                err["code"] = -32603;
                err["message"] = err_msg;
                jsonrpc["error"] = err;
            } else if (response.has("result")) {
                Dictionary raw = response["result"];
                Dictionary formatted = RuntimeBridgeServer::make_response(raw);
                jsonrpc["result"] = formatted;
            }

            mcp_handler_.enqueue_event(jsonrpc);
        });

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

    // Start bridge server
    bridge_server_.start(actual_bridge_port_);

    // Inject dependencies into McpToolRegistry singleton (SDK layer)
    McpToolRegistry *sdk_registry = McpToolRegistry::get_singleton();
    if (!sdk_registry) {
        sdk_registry = memnew(McpToolRegistry);
    }
    sdk_registry->set_handler_registry(&registry_);
    sdk_registry->set_mcp_handler(&mcp_handler_);

    Engine::get_singleton()->register_singleton("McpToolRegistry", sdk_registry);

    // Confirmation dialog for destructive operations
    confirm_dialog_ = memnew(McpConfirmDialog);
    confirm_dialog_->connect("confirmed", Callable(this, "_on_confirm_allow"));
    confirm_dialog_->connect("denied", Callable(this, "_on_confirm_deny"));
    confirm_dialog_->connect("allow_all_session", Callable(this, "_on_confirm_allow_all"));

    mcp_handler_.set_confirm_callback([this](const PendingDestructiveOp &op) {
        pending_dialog_op_ = op;
    });

    http_server_.set_test_engine(&test_engine_);

    logger_.rotate();

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

    mcp_handler_.set_log_callback(nullptr);
    logger_.set_log_callback(nullptr);

    McpToolRegistry *sdk_registry = McpToolRegistry::get_singleton();
    if (sdk_registry) {
        sdk_registry->set_handler_registry(nullptr);
        sdk_registry->set_mcp_handler(nullptr);
    }

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

    bridge_server_.stop();

    http_server_.stop();

    if (g_undo_manager) {
        delete g_undo_manager;
        g_undo_manager = nullptr;
    }

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

    if (pending_dialog_op_.has_value()) {
        confirm_dialog_->show_for_op(
            pending_dialog_op_->jsonrpc_id,
            pending_dialog_op_->tool_name,
            pending_dialog_op_->arguments);
        pending_dialog_op_.reset();
    }

    mcp_handler_.check_pending_timeouts(kConfirmTimeoutMs);

    bridge_server_.poll();
}

}  // namespace godot_mcp
