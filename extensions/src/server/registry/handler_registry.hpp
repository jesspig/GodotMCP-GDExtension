#pragma once

#include <functional>
#include <map>
#include <memory>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

// 前向声明
class ITool;

using CommandFn = std::function<godot::Dictionary(const godot::Dictionary &args)>;

struct ToolInfo {
    godot::String name;
    godot::String description;
    godot::String brief;
    godot::String category;
    godot::String source; // "builtin" | "custom" | "meta" | "built_in"
    godot::Dictionary input_schema;
    bool enabled = true;
};

class HandlerRegistry {
public:
    HandlerRegistry();

    // ── 旧式注册（CommandFn）──
    void register_tool(const godot::String &name, CommandFn fn);
    void register_custom_tool(const godot::String &name, const godot::String &category,
                              const godot::String &brief, const godot::String &description,
                              const godot::Dictionary &schema, CommandFn fn);
    bool unregister_custom_tool(const godot::String &name);

    // ── 新式注册（ITool）──
    void register_tool(std::unique_ptr<ITool> tool);
    // 统一调度入口：先查 itool_table_，再查 table_
    godot::Dictionary execute(const godot::String &name, const godot::Dictionary &args);

    // --- Schema loading ---
    void load_schemas_from_json(const godot::String &json_text);

    // --- Lookup ---
    const CommandFn *find(const godot::String &name) const;
    bool has(const godot::String &name) const;
    int size() const;

    const ToolInfo *find_tool_info(const godot::String &name) const;
    godot::Array get_all_tools() const;
    godot::Array get_enabled_tools() const;
    bool is_tool_enabled(const godot::String &name) const;
    void set_tool_enabled(const godot::String &name, bool enabled);

    // --- Category queries (for progressive disclosure) ---
    godot::Array get_categories() const;
    godot::Array get_tools_in_category(const godot::String &category) const;
    const ToolInfo *get_tool_schema(const godot::String &name) const;

    // --- Always-on tools list ---
    godot::Array get_always_on_tools() const;

    // --- Counts ---
    int builtin_tool_count() const;
    int custom_tool_count() const;

    // --- Version info ---
    void set_engine_version(const godot::String &v) { engine_version_ = v; }
    const godot::String &engine_version() const { return engine_version_; }
    void set_plugin_version(const godot::String &v) { plugin_version_ = v; }
    const godot::String &plugin_version() const { return plugin_version_; }

private:
    godot::Dictionary make_tool_entry(const ToolInfo &info) const;

    godot::HashMap<godot::String, CommandFn> table_;
    std::map<godot::String, std::unique_ptr<ITool>> itool_table_;
    godot::HashMap<godot::String, ToolInfo> tool_info_;
    godot::String engine_version_;
    godot::String plugin_version_;
};

void register_all_tools(HandlerRegistry &reg);
void register_itools(HandlerRegistry &reg);

}  // namespace godot_mcp
