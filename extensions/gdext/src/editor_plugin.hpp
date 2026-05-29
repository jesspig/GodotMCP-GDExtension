#pragma once

#include "commands/handler_registry.hpp"
#include "ipc/http_server.hpp"
#include "ipc/ws_server.hpp"
#include "mcp/mcp_handler.hpp"

#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot_mcp {

class McpEditorPlugin : public godot::EditorPlugin {
    GDCLASS(McpEditorPlugin, godot::EditorPlugin)

public:
    McpEditorPlugin();
    ~McpEditorPlugin() override;

    void _enter_tree() override;
    void _exit_tree() override;

    godot::String _get_plugin_name() const override;

protected:
    static void _bind_methods();

private:
    static int read_port_from_env(const godot::String &env_var, int default_port);

    void _on_process_frame();

    void load_tool_schemas();

    HandlerRegistry registry_;
    McpHandler mcp_handler_{&registry_};
    WsServer ws_server_;
    HttpServer http_server_;
    int ws_port_ = 9500;
    int http_port_ = 9600;
    bool started_ = false;
    uint64_t last_poll_log_ = 0;
};

}  // namespace godot_mcp
