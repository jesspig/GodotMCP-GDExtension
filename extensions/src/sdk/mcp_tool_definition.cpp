#include "mcp_tool_definition.hpp"
#include "mcp_tool_registry.hpp"

using namespace godot;

namespace godot_mcp {

McpToolDefinition::McpToolDefinition() = default;
McpToolDefinition::~McpToolDefinition() = default;

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

String McpToolDefinition::get_tool_name() const { return tool_name_; }
void McpToolDefinition::set_tool_name(const String &v) { tool_name_ = v; }

String McpToolDefinition::get_category() const { return category_; }
void McpToolDefinition::set_category(const String &v) { category_ = v; }

String McpToolDefinition::get_brief() const { return brief_; }
void McpToolDefinition::set_brief(const String &v) { brief_ = v; }

String McpToolDefinition::get_description() const { return description_; }
void McpToolDefinition::set_description(const String &v) { description_ = v; }

Dictionary McpToolDefinition::get_input_schema() const { return input_schema_; }
void McpToolDefinition::set_input_schema(const Dictionary &v) { input_schema_ = v; }

bool McpToolDefinition::get_is_meta() const { return is_meta_; }
void McpToolDefinition::set_is_meta(bool v) { is_meta_ = v; }

// ---------------------------------------------------------------------------
// execute — virtual dispatch via script instance check
// GDScript subclasses override func execute(args: Dictionary) -> Dictionary
// C++ base class instances (no script attached) return an error.
// has_method("execute") is always true due to ClassDB binding (line 103),
// so we check get_script() to distinguish GDScript overrides.
// ---------------------------------------------------------------------------

Dictionary McpToolDefinition::execute(const Dictionary &args) {
    if (get_script().get_type() != Variant::NIL) {
        Variant ret = call("execute", args);
        if (ret.get_type() == Variant::DICTIONARY) {
            return Dictionary(ret);
        }
        Dictionary d;
        d["result"] = ret;
        return d;
    }
    Dictionary d;
    d["error"] = "execute() not implemented";
    return d;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void McpToolDefinition::register_tool() {
    McpToolRegistry *registry = McpToolRegistry::get_singleton();
    if (registry) {
        registry->register_definition(this);
    }
}

void McpToolDefinition::unregister_tool() {
    McpToolRegistry *registry = McpToolRegistry::get_singleton();
    if (registry) {
        registry->unregister_definition(tool_name_);
    }
}

// ---------------------------------------------------------------------------
// Bind methods
// ---------------------------------------------------------------------------

void McpToolDefinition::_bind_methods() {
    // Properties
    ClassDB::bind_method(D_METHOD("get_tool_name"), &McpToolDefinition::get_tool_name);
    ClassDB::bind_method(D_METHOD("set_tool_name", "v"), &McpToolDefinition::set_tool_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tool_name"), "set_tool_name", "get_tool_name");

    ClassDB::bind_method(D_METHOD("get_category"), &McpToolDefinition::get_category);
    ClassDB::bind_method(D_METHOD("set_category", "v"), &McpToolDefinition::set_category);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "category"), "set_category", "get_category");

    ClassDB::bind_method(D_METHOD("get_brief"), &McpToolDefinition::get_brief);
    ClassDB::bind_method(D_METHOD("set_brief", "v"), &McpToolDefinition::set_brief);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "brief"), "set_brief", "get_brief");

    ClassDB::bind_method(D_METHOD("get_description"), &McpToolDefinition::get_description);
    ClassDB::bind_method(D_METHOD("set_description", "v"), &McpToolDefinition::set_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "set_description", "get_description");

    ClassDB::bind_method(D_METHOD("get_input_schema"), &McpToolDefinition::get_input_schema);
    ClassDB::bind_method(D_METHOD("set_input_schema", "v"), &McpToolDefinition::set_input_schema);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "input_schema"), "set_input_schema", "get_input_schema");

    ClassDB::bind_method(D_METHOD("get_is_meta"), &McpToolDefinition::get_is_meta);
    ClassDB::bind_method(D_METHOD("set_is_meta", "v"), &McpToolDefinition::set_is_meta);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_meta"), "set_is_meta", "get_is_meta");

    // execute — exposed as a regular method (GDScript overrides via inheritance)
    ClassDB::bind_method(D_METHOD("execute", "args"), &McpToolDefinition::execute);

    // Registration
    ClassDB::bind_method(D_METHOD("register_tool"), &McpToolDefinition::register_tool);
    ClassDB::bind_method(D_METHOD("unregister_tool"), &McpToolDefinition::unregister_tool);
}

} // namespace godot_mcp
