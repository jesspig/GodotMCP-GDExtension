// =====================================================================
// editor_plugin.hpp — Main EditorPlugin class.
//
// One singleton instance is created when the addon loads. It owns:
//   * the HandlerRegistry (populated once on _enter_tree)
//   * the WsServer (listens on :9500 by default)
//
// _process() runs every frame in the editor and just delegates to
// WsServer::poll(). All command handlers therefore run on the main
// thread, so they can freely call EditorInterface, modify the scene
// tree, register undo steps, etc.
// =====================================================================

#pragma once

#include "commands/handler_registry.hpp"
#include "ipc/ws_server.hpp"

#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot_mcp {

class McpEditorPlugin : public godot::EditorPlugin {
    GDCLASS(McpEditorPlugin, godot::EditorPlugin)

public:
    McpEditorPlugin();
    ~McpEditorPlugin() override;

    // EditorPlugin lifecycle.
    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    godot::String _get_plugin_name() const override;

protected:
    static void _bind_methods();

private:
    // Read GODOT_MCP_PORT environment variable, falling back to 9500.
    static int read_port_from_env();

    HandlerRegistry registry_;
    WsServer ws_server_;
    bool started_ = false;
};

}  // namespace godot_mcp
