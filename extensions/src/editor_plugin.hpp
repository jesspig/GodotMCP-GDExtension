#pragma once

#include "server/registry/handler_registry.hpp"
#include "server/ipc/http_server.hpp"
#include "server/mcp/mcp_handler.hpp"
#include "plugin/test_runner_dock.hpp"
#include "testing/test_engine.hpp"

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

    HandlerRegistry registry_;
    McpHandler mcp_handler_{&registry_};
    HttpServer http_server_;
    TestEngine test_engine_{&registry_};
    TestRunnerDock *test_dock_ = nullptr;
    int http_port_ = 9600;
    bool started_ = false;
};

}  // namespace godot_mcp
