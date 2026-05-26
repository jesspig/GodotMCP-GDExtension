// =====================================================================
// editor_plugin.hpp — Main EditorPlugin class.
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

    void _enter_tree() override;
    void _exit_tree() override;

    godot::String _get_plugin_name() const override;

protected:
    static void _bind_methods();

private:
    static int read_port_from_env();

    // Called every frame via SceneTree::process_frame — survives play mode.
    void _on_process_frame();

    HandlerRegistry registry_;
    WsServer ws_server_;
    bool started_ = false;
};

}  // namespace godot_mcp
