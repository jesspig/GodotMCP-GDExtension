// =====================================================================
// commands/meta.cpp — ping, get_engine_version, get_plugin_version.
//
// Mirrors crates/gdext/src/commands/meta.rs. These are the simplest
// commands: they touch no Godot API and just echo back state captured
// at startup.
// =====================================================================

#include "cmd_utils.hpp"
#include "handler_registry.hpp"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace godot_mcp {

namespace {

Dictionary cmd_ping(const Dictionary & /*args*/) {
    Dictionary d;
    d["message"] = "pong";
    return d;
}

}  // namespace

void register_meta(HandlerRegistry &reg) {
    // ping is stateless.
    reg.register_tool("ping", cmd_ping);

    // get_engine_version reads the value captured when the EditorPlugin
    // started. Capturing inside the lambda keeps the registry decoupled
    // from McpEditorPlugin.
    const String engine_version = reg.engine_version();
    reg.register_tool("get_engine_version",
                      [engine_version](const Dictionary &) {
                          Dictionary d;
                          d["engine_version"] = engine_version;
                          return d;
                      });

    const String plugin_version = reg.plugin_version();
    reg.register_tool("get_plugin_version",
                      [plugin_version](const Dictionary &) {
                          Dictionary d;
                          d["plugin_version"] = plugin_version;
                          return d;
                      });
}

}  // namespace godot_mcp
