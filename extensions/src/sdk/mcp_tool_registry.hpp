#pragma once

#include "mcp_tool_definition.hpp"
#include "server/registry/handler_registry.hpp"
#include "server/mcp/mcp_handler.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class McpToolRegistry : public godot::Object {
    GDCLASS(McpToolRegistry, godot::Object)

public:
    static McpToolRegistry *get_singleton();

    McpToolRegistry();
    ~McpToolRegistry() override;

    // --- Injected dependencies (set by McpEditorPlugin) ---
    void set_handler_registry(HandlerRegistry *reg);
    void set_mcp_handler(McpHandler *handler);

    // --- Mode A: Register from McpToolDefinition ---
    void register_definition(McpToolDefinition *tool_def);
    bool unregister_definition(const godot::String &name);

    // --- Mode B: Register with Callable ---
    void register_tool(
        const godot::String &name,
        const godot::String &category,
        const godot::String &brief,
        const godot::String &description,
        const godot::Dictionary &input_schema,
        const godot::Callable &handler,
        bool is_meta = false,
        bool supports_undo = false,
        bool is_destructive = false);
    bool unregister_tool(const godot::String &name);

    // --- Query ---
    bool has_tool(const godot::String &name) const;
    godot::Array get_custom_tools() const;
    int get_custom_tool_count() const;

    // --- Signals ---
    // tool_registered(name: String)
    // tool_unregistered(name: String)

protected:
    static void _bind_methods();

private:
    static McpToolRegistry *singleton_;

    struct CustomTool {
        godot::String original_name;
        godot::String registered_name;
        godot::String category;
        godot::String brief;
        godot::String description;
        godot::Dictionary input_schema;
        bool is_meta = false;
        bool supports_undo = false;
        bool is_destructive = false;
    };

    godot::HashMap<godot::String, CustomTool> tools_;
    HandlerRegistry *handler_registry_ = nullptr;
    McpHandler *mcp_handler_ = nullptr;

    // Resolve name: add custom_ prefix if not already present
    static godot::String resolve_name(const godot::String &name);
    void notify_tools_changed();
};

} // namespace godot_mcp
