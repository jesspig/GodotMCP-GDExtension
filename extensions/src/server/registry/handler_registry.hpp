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
class RuntimeBridgeServer;
class McpHandler;

using CommandFn = std::function<godot::Dictionary(const godot::Dictionary &args)>;

struct ToolInfo {
    bool is_destructive = false;
    bool is_custom = false;
    bool enabled = true;
    const ITool *tool_ptr = nullptr;
};

class HandlerRegistry {
public:
    HandlerRegistry();
    ~HandlerRegistry();

    [[nodiscard]] bool unregister_custom_tool(const godot::String &name);

    void register_tool(std::unique_ptr<ITool> tool, bool is_custom = false);
    [[nodiscard]] godot::Dictionary execute(const godot::String &name, const godot::Dictionary &args, const godot::Variant &jsonrpc_id = {});

    [[nodiscard]] const ToolInfo *find_tool_info(const godot::String &name) const;

    // --- Category queries (for progressive disclosure) ---
    [[nodiscard]] godot::Array get_categories() const;
    [[nodiscard]] godot::Array get_tools_in_category(const godot::String &category) const;
    [[nodiscard]] godot::PackedStringArray get_all_category_paths() const;

    // --- Always-on tools list ---
    [[nodiscard]] godot::Array get_always_on_tools() const;

    // --- Counts ---
    [[nodiscard]] int builtin_tool_count() const;
    [[nodiscard]] int custom_tool_count() const;

    // --- Runtime bridge ---
    void set_runtime_bridge(RuntimeBridge *bridge) { runtime_bridge_ = bridge; }
    RuntimeBridge *get_runtime_bridge() const { return runtime_bridge_; }

    void set_runtime_bridge_server(RuntimeBridgeServer *server) { bridge_server_ = server; }
    RuntimeBridgeServer *get_runtime_bridge_server() const { return bridge_server_; }

    // --- Search engine ---
    [[nodiscard]] godot::Array search_tools(const godot::String &query, const godot::String &category = "", int limit = 20) const;
    void record_tool_call(const godot::String &name);
    [[nodiscard]] godot::Array get_search_suggestions(const godot::String &prefix, int limit = 10) const;

    // --- Tools changed callback ---
    using ToolsChangedCallback = std::function<void()>;
    void set_on_tools_changed(ToolsChangedCallback cb) { on_tools_changed_ = std::move(cb); }
    void notify_tools_changed();

    // --- Version info ---
    void set_engine_version(const godot::String &v) { engine_version_ = v; }
    const godot::String &engine_version() const { return engine_version_; }
    void set_plugin_version(const godot::String &v) { plugin_version_ = v; }
    const godot::String &plugin_version() const { return plugin_version_; }

private:
    godot::Dictionary make_tool_entry(const godot::String &name, const ToolInfo &info) const;

    const ITool *find_itool(const godot::String &name) const {
        auto it = itool_table_.find(name);
        return (it != itool_table_.end()) ? it->second.get() : nullptr;
    }

    void apply_freq_decay();

    static godot::PackedStringArray tokenize(const godot::String &text);

    // 使用 std::map 而非 godot::HashMap，因�?unique_ptr 不可复制（HashMap 要求 value 可复制）
    std::map<godot::String, std::unique_ptr<ITool>> itool_table_;
    godot::HashMap<godot::String, ToolInfo> tool_info_;

    // 预构建搜索索引：在 register_tool() 时增量构建，避免 search_tools() 实时 tokenize
    struct SearchIndexEntry {
        godot::PackedStringArray tokens;
    };
    godot::HashMap<godot::String, SearchIndexEntry> search_index_;

    mutable godot::Array categories_cache_;
    mutable bool categories_dirty_ = true;
    mutable godot::HashMap<godot::String, int> freq_index_;
    mutable int64_t freq_decay_counter_ = 0;
    RuntimeBridge *runtime_bridge_ = nullptr;
    RuntimeBridgeServer *bridge_server_ = nullptr;
    godot::String engine_version_;
    godot::String plugin_version_;
    ToolsChangedCallback on_tools_changed_;
};

void register_itools(HandlerRegistry &reg);

}  // namespace godot_mcp
