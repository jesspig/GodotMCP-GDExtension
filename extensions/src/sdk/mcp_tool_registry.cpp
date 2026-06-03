#include "mcp_tool_registry.hpp"
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

    // Build CommandFn wrapper around GDScript Callable
    // We capture a Ref to prevent the definition from being garbage collected
    Ref<McpToolDefinition> def_ref = Ref<McpToolDefinition>(tool_def);
    CommandFn wrapper = [def_ref](const Dictionary &args) -> Dictionary {
        return def_ref->execute(args);
    };

    // Store in our map
    CustomTool ct;
    ct.original_name = original;
    ct.registered_name = resolved;
    ct.category = tool_def->get_category();
    ct.brief = tool_def->get_brief();
    ct.description = tool_def->get_description();
    ct.input_schema = tool_def->get_input_schema();
    ct.is_meta = tool_def->get_is_meta();
    tools_[resolved] = ct;

    // Register in HandlerRegistry
    if (handler_registry_) {
        handler_registry_->register_custom_tool(
            resolved, ct.category, ct.brief, ct.description, ct.input_schema,
            std::move(wrapper), ct.is_meta);
    }

    log_info("sdk", String("Registered custom tool: ") + resolved);
    emit_signal("tool_registered", resolved);
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
    bool is_meta) {

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

    // Build CommandFn wrapper around Callable
    Callable captured_handler = handler;
    CommandFn wrapper = [captured_handler](const Dictionary &args) -> Dictionary {
        Variant ret = captured_handler.call(args);
        if (ret.get_type() == Variant::DICTIONARY) {
            return Dictionary(ret);
        }
        Dictionary d;
        d["result"] = ret;
        return d;
    };

    // Store in our map
    CustomTool ct;
    ct.original_name = name;
    ct.registered_name = resolved;
    ct.category = category;
    ct.brief = brief;
    ct.description = description;
    ct.input_schema = input_schema;
    ct.is_meta = is_meta;
    tools_[resolved] = ct;

    // Register in HandlerRegistry
    if (handler_registry_) {
        handler_registry_->register_custom_tool(
            resolved, category, brief, description, input_schema,
            std::move(wrapper), is_meta);
    }

    log_info("sdk", String("Registered custom tool: ") + resolved);
    emit_signal("tool_registered", resolved);
    notify_tools_changed();
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
    emit_signal("tool_unregistered", resolved);
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
                                  "is_meta"),
                         &McpToolRegistry::register_tool, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("unregister_tool", "name"),
                         &McpToolRegistry::unregister_tool);

    // Query
    ClassDB::bind_method(D_METHOD("has_tool", "name"),
                         &McpToolRegistry::has_tool);
    ClassDB::bind_method(D_METHOD("get_custom_tools"),
                         &McpToolRegistry::get_custom_tools);
    ClassDB::bind_method(D_METHOD("get_custom_tool_count"),
                         &McpToolRegistry::get_custom_tool_count);

    // Signals
    ADD_SIGNAL(MethodInfo("tool_registered",
                          PropertyInfo(Variant::STRING, "name")));
    ADD_SIGNAL(MethodInfo("tool_unregistered",
                          PropertyInfo(Variant::STRING, "name")));
}

} // namespace godot_mcp
