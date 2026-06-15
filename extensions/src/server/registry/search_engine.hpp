#pragma once
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot_mcp {

struct ToolInfo;

class SearchEngine {
public:
    SearchEngine() = default;

    void set_tool_info(const godot::HashMap<godot::String, ToolInfo> *info) { tool_info_ = info; }

    godot::Array search_tools(const godot::String &query, const godot::String &category = "", int limit = 20) const;
    godot::Array get_search_suggestions(const godot::String &prefix, int limit = 10) const;
    void record_tool_call(const godot::String &tool_name);
    godot::Vector<godot::String> get_most_called_tools(int count = 10) const;

private:
    static godot::PackedStringArray tokenize(const godot::String &text);

    const godot::HashMap<godot::String, ToolInfo> *tool_info_ = nullptr;
    godot::HashMap<godot::String, int64_t> call_frequency_;
};

} // namespace godot_mcp
