#pragma once

#include <functional>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

using CommandFn = std::function<godot::Dictionary(const godot::Dictionary &args)>;

struct ToolInfo {
    godot::String name;
    godot::String description;
    godot::Dictionary input_schema;
    bool enabled = true;
};

class HandlerRegistry {
public:
    HandlerRegistry();

    void register_tool(const godot::String &name, CommandFn fn);
    void register_tool_info(const godot::String &name, const godot::String &description,
                            const godot::Dictionary &input_schema);
    void load_schemas_from_json(const godot::String &json_text);

    const CommandFn *find(const godot::String &name) const;
    bool has(const godot::String &name) const;
    int size() const;

    const ToolInfo *find_tool_info(const godot::String &name) const;
    godot::Array get_all_tools() const;
    godot::Array get_enabled_tools() const;
    bool is_tool_enabled(const godot::String &name) const;
    void set_tool_enabled(const godot::String &name, bool enabled);

    void set_engine_version(const godot::String &v) { engine_version_ = v; }
    const godot::String &engine_version() const { return engine_version_; }
    void set_plugin_version(const godot::String &v) { plugin_version_ = v; }
    const godot::String &plugin_version() const { return plugin_version_; }

private:
    godot::HashMap<godot::String, CommandFn> table_;
    godot::HashMap<godot::String, ToolInfo> tool_info_;
    godot::String engine_version_;
    godot::String plugin_version_;
};

void register_all_tools(HandlerRegistry &reg);

}  // namespace godot_mcp
