// =====================================================================
// register_types.cpp — GDExtension entry point.
//
// Registers SDK classes (McpToolDefinition, McpToolRegistry) and the
// McpEditorPlugin at MODULE_INITIALIZATION_LEVEL_EDITOR.
// =====================================================================

#include "editor_plugin.hpp"
#include "runtime/game_bridge.hpp"
#include "sdk/mcp_tool_definition.hpp"
#include "sdk/mcp_tool_registry.hpp"
#include "ui/mcp_console.hpp"
#include "ui/mcp_dock.hpp"

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static void initialize_godot_mcp(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_CLASS(godot_mcp::GameBridgeNode);

        if (!godot::Engine::get_singleton()->is_editor_hint()) {
            godot_mcp::GameBridgeNode *bridge = memnew(godot_mcp::GameBridgeNode);
            bridge->set_name("GameBridgeNode");
            bridge->call_deferred("_self_add");
        }
        return;
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }
    GDREGISTER_CLASS(godot_mcp::McpToolDefinition);
    GDREGISTER_CLASS(godot_mcp::McpToolRegistry);
    GDREGISTER_CLASS(godot_mcp::McpConsole);
    GDREGISTER_CLASS(godot_mcp::McpDock);

    GDREGISTER_CLASS(godot_mcp::McpEditorPlugin);
    EditorPlugins::add_by_type<godot_mcp::McpEditorPlugin>();
}

static void uninitialize_godot_mcp(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }
    EditorPlugins::remove_by_type<godot_mcp::McpEditorPlugin>();
}

extern "C" {

GDExtensionBool GDE_EXPORT gdext_mcp_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_godot_mcp);
    init_obj.register_terminator(uninitialize_godot_mcp);
    init_obj.set_minimum_library_initialization_level(
            MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}

}  // extern "C"
