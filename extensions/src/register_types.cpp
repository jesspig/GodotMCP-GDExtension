// =====================================================================
// register_types.cpp — GDExtension entry point.
//
// Registers SDK classes (McpToolDefinition, McpToolRegistry) and the
// McpEditorPlugin at MODULE_INITIALIZATION_LEVEL_EDITOR.
// Entry symbol kept as `gdext_rust_init` for .gdextension compatibility.
// =====================================================================

#include "editor_plugin.hpp"
#include "sdk/mcp_tool_definition.hpp"
#include "sdk/mcp_tool_registry.hpp"

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static void initialize_godot_mcp(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }
    // SDK classes (GDScript/C# can inherit these)
    GDREGISTER_CLASS(godot_mcp::McpToolDefinition);
    GDREGISTER_CLASS(godot_mcp::McpToolRegistry);

    // Editor plugin
    GDREGISTER_CLASS(godot_mcp::McpEditorPlugin);
    EditorPlugins::add_by_type<godot_mcp::McpEditorPlugin>();
}

static void uninitialize_godot_mcp(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }
    EditorPlugins::remove_by_type<godot_mcp::McpEditorPlugin>();
}

extern "C" {

// Entry symbol referenced by godot_mcp.gdextension.
// Kept as `gdext_rust_init` to avoid changing the existing config file.
GDExtensionBool GDE_EXPORT gdext_rust_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_godot_mcp);
    init_obj.register_terminator(uninitialize_godot_mcp);
    init_obj.set_minimum_library_initialization_level(
            MODULE_INITIALIZATION_LEVEL_EDITOR);

    return init_obj.init();
}

}  // extern "C"
