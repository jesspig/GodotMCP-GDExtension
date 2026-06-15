#include "mcp_tool_registry.hpp"
#include "built_in/tool_adapter.hpp"
#include "logging.hpp"

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace godot_mcp {

McpToolRegistry *McpToolRegistry::singleton_ = nullptr;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

McpToolRegistry *McpToolRegistry::get_singleton() {
    return singleton_;
}

McpToolRegistry::McpToolRegistry() {
    singleton_ = this;
}

McpToolRegistry::~McpToolRegistry() {
    if (singleton_ == this) {
        singleton_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Dependency injection
// ---------------------------------------------------------------------------

void McpToolRegistry::set_handler_registry(HandlerRegistry *reg) {
    handler_registry_ = reg;
}

void McpToolRegistry::set_mcp_handler(McpHandler *handler) {
    mcp_handler_ = handler;
}

// ---------------------------------------------------------------------------
// Name resolution: auto-add "custom_" prefix
// ---------------------------------------------------------------------------

String McpToolRegistry::resolve_name(const String &name) {
    if (name.begins_with("custom_")) {
        return name;
    }
    return String("custom_") + name;
}

// ---------------------------------------------------------------------------
// Mode A: Register from McpToolDefinition (RefCounted)
// ---------------------------------------------------------------------------

void McpToolRegistry::register_definition(McpToolDefinition *tool_def) {
    if (!tool_def) return;

    const String original = tool_def->get_tool_name();
    if (original.is_empty()) {
        log_warn("sdk", "McpToolDefinition has empty tool_name, skipping registration");
        return;
    }

    const String resolved = resolve_name(original);

    // Warn on conflict
    if (tools_.has(resolved)) {
        log_warn("sdk", String("Custom tool '") + resolved +
                         String("' already registered — overwriting"));
    }

    // Store custom tool metadata
    CustomTool ct;
    ct.original_name = original;
    ct.registered_name = resolved;
    ct.category = tool_def->get_category();
    ct.brief = tool_def->get_brief();
    ct.description = tool_def->get_description();
    ct.input_schema = tool_def->get_input_schema();
    ct.is_meta = tool_def->get_is_meta();
    ct.supports_undo = tool_def->get_supports_undo();
    ct.is_destructive = tool_def->get_is_destructive();
    tools_[resolved] = ct;

    // Register via IToolAdapter — goes into itool_table_
    if (handler_registry_) {
        Ref<McpToolDefinition> def_ref = Ref<McpToolDefinition>(tool_def);
        CommandFn wrapper = [def_ref](const Dictionary &args) -> Dictionary {
            return def_ref->execute(args);
        };

        auto adapter = std::make_unique<IToolAdapter>(
            resolved,
            ct.category,
            ct.brief,
            ct.description,
            ct.input_schema,
            std::move(wrapper),
            ct.is_meta,
            false, // needs_scene
            false, // needs_node
            ct.supports_undo,
            ct.is_destructive);

        handler_registry_->register_tool(std::move(adapter), true);
    }

    log_info("sdk", String("Registered custom tool: ") + resolved);
    notify_tools_changed();
}

bool McpToolRegistry::unregister_definition(const String &name) {
    const String resolved = resolve_name(name);
    return unregister_tool(resolved);
}

// ---------------------------------------------------------------------------
// Mode B: Register with Callable
// ---------------------------------------------------------------------------

void McpToolRegistry::register_tool(
    const String &name,
    const String &category,
    const String &brief,
    const String &description,
    const Dictionary &input_schema,
    const Callable &handler,
    bool is_meta,
    bool supports_undo,
    bool is_destructive) {

    if (name.is_empty()) {
        log_warn("sdk", "register_tool called with empty name, skipping");
        return;
    }

    const String resolved = resolve_name(name);

    // Warn on conflict
    if (tools_.has(resolved)) {
        log_warn("sdk", String("Custom tool '") + resolved +
                         String("' already registered — overwriting"));
    }

    // Store metadata
    CustomTool ct;
    ct.original_name = name;
    ct.registered_name = resolved;
    ct.category = category;
    ct.brief = brief;
    ct.description = description;
    ct.input_schema = input_schema;
    ct.is_meta = is_meta;
    ct.supports_undo = supports_undo;
    ct.is_destructive = is_destructive;
    tools_[resolved] = ct;

    // Register via IToolAdapter
    if (handler_registry_) {
        Callable captured_handler = handler;
        auto adapter = std::make_unique<IToolAdapter>(
            resolved, category, brief, description, input_schema,
            captured_handler, is_meta, false, false, supports_undo, is_destructive);
        handler_registry_->register_tool(std::move(adapter), true);
    }

    log_info("sdk", String("Registered custom tool: ") + resolved);
    notify_tools_changed();
}

// ---------------------------------------------------------------------------
// Mode B (simplified): Register with Callable — no description / flags
// ---------------------------------------------------------------------------

void McpToolRegistry::register_tool_simple(
    const String &name,
    const String &category,
    const String &brief,
    const Dictionary &input_schema,
    const Callable &handler) {
    register_tool(name, category, brief, String(), input_schema, handler,
                  false, false, false);
}

bool McpToolRegistry::unregister_tool(const String &name) {
    const String resolved = resolve_name(name);

    if (!tools_.has(resolved)) {
        return false;
    }

    tools_.erase(resolved);

    if (handler_registry_) {
        handler_registry_->unregister_custom_tool(resolved);
    }

    log_info("sdk", String("Unregistered custom tool: ") + resolved);
    notify_tools_changed();
    return true;
}

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

bool McpToolRegistry::has_tool(const String &name) const {
    const String resolved = resolve_name(name);
    return tools_.has(resolved);
}

Array McpToolRegistry::get_custom_tools() const {
    Array result;
    for (const KeyValue<String, CustomTool> &kv : tools_) {
        Dictionary d;
        d["name"] = kv.value.registered_name;
        d["original_name"] = kv.value.original_name;
        d["category"] = kv.value.category;
        d["brief"] = kv.value.brief;
        d["description"] = kv.value.description;
        d["input_schema"] = kv.value.input_schema;
        d["is_meta"] = kv.value.is_meta;
        d["supports_undo"] = kv.value.supports_undo;
        d["is_destructive"] = kv.value.is_destructive;
        result.push_back(d);
    }
    return result;
}

int McpToolRegistry::get_custom_tool_count() const {
    return tools_.size();
}

// ---------------------------------------------------------------------------
// Notify MCP clients of tool list changes
// ---------------------------------------------------------------------------

void McpToolRegistry::notify_tools_changed() {
    if (mcp_handler_) {
        mcp_handler_->notify_tools_list_changed();
    }
}

// ---------------------------------------------------------------------------
// Bind methods
// ---------------------------------------------------------------------------

void McpToolRegistry::_bind_methods() {
    // Singleton accessor
    ClassDB::bind_static_method("McpToolRegistry",
                                D_METHOD("get_singleton"),
                                &McpToolRegistry::get_singleton);

    // Mode A
    ClassDB::bind_method(D_METHOD("register_definition", "tool_def"),
                         &McpToolRegistry::register_definition);
    ClassDB::bind_method(D_METHOD("unregister_definition", "name"),
                         &McpToolRegistry::unregister_definition);

    // Mode B
    ClassDB::bind_method(D_METHOD("register_tool", "name", "category", "brief",
                                  "description", "input_schema", "handler",
                                  "is_meta", "supports_undo", "is_destructive"),
                         &McpToolRegistry::register_tool, DEFVAL(false), DEFVAL(false), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("register_tool_simple", "name", "category",
                                  "brief", "input_schema", "handler"),
                         &McpToolRegistry::register_tool_simple);
    ClassDB::bind_method(D_METHOD("unregister_tool", "name"),
                         &McpToolRegistry::unregister_tool);

    // Query
    ClassDB::bind_method(D_METHOD("has_tool", "name"),
                         &McpToolRegistry::has_tool);
    ClassDB::bind_method(D_METHOD("get_custom_tools"),
                         &McpToolRegistry::get_custom_tools);
    ClassDB::bind_method(D_METHOD("get_custom_tool_count"),
                         &McpToolRegistry::get_custom_tool_count);

    // Signals (currently unused, removed to avoid dead code)
}

} // namespace godot_mcp
