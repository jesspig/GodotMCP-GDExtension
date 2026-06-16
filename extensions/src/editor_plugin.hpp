#pragma once

#include "runtime/bridge.hpp"
#include "server/registry/handler_registry.hpp"
#include "server/ipc/http_server.hpp"
#include "server/mcp/mcp_handler.hpp"
#include "testing/test_engine.hpp"
#include "ui/mcp_logger.hpp"

#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot_mcp {

class McpDock;
class McpConsole;

class McpEditorPlugin : public godot::EditorPlugin {
    GDCLASS(McpEditorPlugin, godot::EditorPlugin)

public:
    McpEditorPlugin();
    ~McpEditorPlugin() override;

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    godot::String _get_plugin_name() const override;

    void set_http_port(int port) { http_port_ = port; }
    void set_bridge_port(int port) { bridge_port_ = port; }
    void set_http_host(const godot::String &host) { http_host_ = host; }
    int http_port() const { return http_port_; }
    int bridge_port() const { return bridge_port_; }
    const godot::String &http_host() const { return http_host_; }
    bool is_started() const { return started_; }
    bool is_bridge_connected() const { return runtime_bridge_.is_connected(); }

    void save_config();
    void restart_server();

protected:
    static void _bind_methods();

private:
    static int read_port_from_env(const godot::String &env_var, int default_port);

    void _try_bridge_connect();

    HandlerRegistry registry_;
    McpHandler mcp_handler_{&registry_};
    HttpServer http_server_;
    TestEngine test_engine_{&registry_};
    RuntimeBridge runtime_bridge_;
    McpLogger logger_;
    McpDock *mcp_dock_ = nullptr;
    McpConsole *mcp_console_ = nullptr;
    int http_port_ = 9600;
    int bridge_port_ = 9601;
    godot::String http_host_ = "127.0.0.1";
    bool started_ = false;
    bool game_was_running_ = false;


    void load_config();
};

}  // namespace godot_mcp
