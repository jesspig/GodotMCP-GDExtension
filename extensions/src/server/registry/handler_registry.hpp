#pragma once

#include <functional>
#include <map>
#include <memory>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_mcp {

// 前向声明
class ITool;
class RuntimeBridge;

using CommandFn = std::function<godot::Dictionary(const godot::Dictionary &args)>;

struct ToolInfo {
    godot::String name;
    godot::String description;
    godot::String brief;
    godot::String category;
    godot::String category_description;
    bool is_meta = false;
    bool supports_undo = false;
    bool is_destructive = false;
    bool is_custom = false;
    godot::Dictionary input_schema;
    bool enabled = true;
};

class HandlerRegistry {
public:
    HandlerRegistry();
    ~HandlerRegistry();

    bool unregister_custom_tool(const godot::String &name);

    void register_tool(std::unique_ptr<ITool> tool, bool is_custom = false);
    void finalize_registration(); // call after all tools are registered
    godot::Dictionary execute(const godot::String &name, const godot::Dictionary &args);

    const ToolInfo *find_tool_info(const godot::String &name) const;

    // --- Category queries (for progressive disclosure) ---
    godot::Array get_categories() const;
    godot::Array get_tools_in_category(const godot::String &category) const;

    // --- Always-on tools list ---
    godot::Array get_always_on_tools() const;

    // --- Counts ---
    int builtin_tool_count() const;
    int custom_tool_count() const;

    // --- Runtime bridge ---
    void set_runtime_bridge(RuntimeBridge *bridge) { runtime_bridge_ = bridge; }
    RuntimeBridge *get_runtime_bridge() const { return runtime_bridge_; }

    // --- Search engine ---
    godot::Array search_tools(const godot::String &query, const godot::String &category = "", int limit = 20) const;
    void record_tool_call(const godot::String &name);
    godot::Array get_search_suggestions(const godot::String &prefix, int limit = 10) const;

    // --- Version info ---
    void set_engine_version(const godot::String &v) { engine_version_ = v; }
    const godot::String &engine_version() const { return engine_version_; }
    void set_plugin_version(const godot::String &v) { plugin_version_ = v; }
    const godot::String &plugin_version() const { return plugin_version_; }

private:
    godot::Dictionary make_tool_entry(const ToolInfo &info) const;

    static godot::PackedStringArray tokenize(const godot::String &text);
    void rebuild_search_index();

    // 使用 std::map 而非 godot::HashMap，因为 unique_ptr 不可复制（HashMap 要求 value 可复制）
    std::map<godot::String, std::unique_ptr<ITool>> itool_table_;
    godot::HashMap<godot::String, ToolInfo> tool_info_;
    godot::HashMap<godot::String, godot::Array> search_index_;
    godot::HashMap<godot::String, int> freq_index_;
    RuntimeBridge *runtime_bridge_ = nullptr;
    godot::String engine_version_;
    godot::String plugin_version_;
};

void register_itools(HandlerRegistry &reg);

}  // namespace godot_mcp
