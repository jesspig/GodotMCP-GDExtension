// =====================================================================
// commands/handler_registry.hpp — Tool name -> command function table.
//
// Each command exposed over the MCP protocol is registered as a free
// function returning a Dictionary. The WebSocket dispatch loop looks
// up the tool name in this registry, executes the handler, and wraps
// the result into an IpcResponse.
//
// All handlers run on the main thread inside EditorPlugin::_process(),
// so they may freely call any EditorInterface / scene-tree API.
// =====================================================================

#pragma once

#include <functional>

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

using CommandFn = std::function<godot::Dictionary(const godot::Dictionary &args)>;

class HandlerRegistry {
public:
    HandlerRegistry();

    // Register a single tool. Last registration wins (used by groups).
    void register_tool(const godot::String &name, CommandFn fn);

    // Lookup. Returns nullptr when the tool is not registered.
    const CommandFn *find(const godot::String &name) const;

    // Returns true if the tool name is registered.
    bool has(const godot::String &name) const;

    // Number of registered tools (mostly for diagnostics / tests).
    int size() const;

    // Engine version string, injected by McpEditorPlugin at startup.
    void set_engine_version(const godot::String &v) { engine_version_ = v; }
    const godot::String &engine_version() const { return engine_version_; }

    // Plugin version string baked in at compile time.
    void set_plugin_version(const godot::String &v) { plugin_version_ = v; }
    const godot::String &plugin_version() const { return plugin_version_; }

private:
    godot::HashMap<godot::String, CommandFn> table_;
    godot::String engine_version_;
    godot::String plugin_version_;
};

// Populate the registry with every tool group. Implemented in
// handler_registry.cpp — each cmd_*.cpp file adds an
// `register_<group>(HandlerRegistry &)` free function.
void register_all_tools(HandlerRegistry &reg);

}  // namespace godot_mcp
