#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

class McpToolDefinition : public godot::RefCounted {
    GDCLASS(McpToolDefinition, godot::RefCounted)

public:
    McpToolDefinition();
    ~McpToolDefinition() override;

    // --- Properties (set in GDScript _init()) ---
    godot::String get_tool_name() const;
    void set_tool_name(const godot::String &v);

    godot::String get_category() const;
    void set_category(const godot::String &v);

    godot::String get_brief() const;
    void set_brief(const godot::String &v);

    godot::String get_description() const;
    void set_description(const godot::String &v);

    godot::Dictionary get_input_schema() const;
    void set_input_schema(const godot::Dictionary &v);

    // --- Core virtual (GDScript overrides this) ---
    // func execute(args: Dictionary) -> Dictionary
    virtual godot::Dictionary execute(const godot::Dictionary &args);

    // --- Registration ---
    void register_tool();
    void unregister_tool();

protected:
    static void _bind_methods();

private:
    godot::String tool_name_;
    godot::String category_;
    godot::String brief_;
    godot::String description_;
    godot::Dictionary input_schema_;
};

} // namespace godot_mcp
