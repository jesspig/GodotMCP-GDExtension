#include "handler_registry.hpp"
#include "built_in/tool_base.hpp"

#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace godot_mcp {

HandlerRegistry::HandlerRegistry() = default;

// ---------------------------------------------------------------------------
// Tool registration
// ---------------------------------------------------------------------------

void HandlerRegistry::register_tool(const String &name, CommandFn fn) {
    table_[name] = std::move(fn);
}

void HandlerRegistry::register_custom_tool(const String &name, const String &category,
                                           const String &brief, const String &description,
                                           const Dictionary &schema, CommandFn fn,
                                           bool is_meta) {
    table_[name] = std::move(fn);

    ToolInfo info;
    info.name = name;
    info.category = category;
    info.brief = brief;
    info.description = description;
    info.input_schema = schema;
    info.is_meta = is_meta;
    info.is_custom = true;
    info.enabled = true;
    tool_info_[name] = info;
}

bool HandlerRegistry::unregister_custom_tool(const String &name) {
    auto it_info = tool_info_.find(name);
    if (it_info == tool_info_.end() || !it_info->value.is_custom) {
        return false;
    }
    tool_info_.erase(name);
    table_.erase(name);
    return true;
}

// ---------------------------------------------------------------------------
// ITool registration
// ---------------------------------------------------------------------------

void HandlerRegistry::register_tool(std::unique_ptr<ITool> tool) {
    if (!tool) return;

    // 注入 registry 指针（meta 工具需要它回调查询）
    tool->set_registry(this);

    const String name = tool->registered_name();

    // Populate ToolInfo for backward compatibility (category queries, schema, etc.)
    ToolInfo info;
    info.name = name;
    info.category = tool->category();
    info.brief = tool->brief();
    info.description = tool->description();
    info.category_description = tool->category_description();
    info.input_schema = tool->input_schema();
    info.is_meta = tool->is_meta();
    info.is_custom = false;
    info.enabled = true;
    tool_info_[name] = info;

    itool_table_.emplace(name, std::move(tool));
}

// ---------------------------------------------------------------------------
// Unified execution: ITool first, then CommandFn fallback
// ---------------------------------------------------------------------------

Dictionary HandlerRegistry::execute(const String &name, const Dictionary &args) {
    // Check ITool table first
    auto it = itool_table_.find(name);
    if (it != itool_table_.end()) {
        return it->second->execute(args);
    }

    // Fall back to CommandFn table
    auto fn_it = table_.find(name);
    if (fn_it != table_.end()) {
        return fn_it->value(args);
    }

    // Tool not found
    Dictionary error;
    error["error"] = String("Tool not found: ") + name;
    return error;
}

// ---------------------------------------------------------------------------
// Schema loading (builtin tools from JSON)
// ---------------------------------------------------------------------------

void HandlerRegistry::load_schemas_from_json(const String &json_text) {
    Ref<JSON> json;
    json.instantiate();
    const Error err = json->parse(json_text);
    if (err != OK) {
        return;
    }
    const Variant data = json->get_data();
    if (data.get_type() != Variant::ARRAY) return;
    const Array tools = data;
    for (int i = 0; i < tools.size(); ++i) {
        const Dictionary entry = tools[i];
        const String name = entry.get("name", "");
        if (name.is_empty()) continue;

        // 不覆盖已通过 ITool 注册或自定义注册的工具信息
        {
            auto existing = tool_info_.find(name);
            if (existing != tool_info_.end() && (existing->value.is_meta || existing->value.is_custom)) continue;
        }

        ToolInfo info;
        info.name = name;
        info.description = entry.get("description", "");
        info.brief = entry.get("brief", "");
        info.category = entry.get("category", "");
        info.is_meta = false;
        info.is_custom = false;
        info.input_schema = entry.get("input_schema", Dictionary());
        info.enabled = true;
        tool_info_[name] = info;
    }
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

const ToolInfo *HandlerRegistry::find_tool_info(const String &name) const {
    auto it = tool_info_.find(name);
    if (it == tool_info_.end()) return nullptr;
    return &it->value;
}

Dictionary HandlerRegistry::make_tool_entry(const ToolInfo &info) const {
    Dictionary d;
    d["name"] = info.name;
    d["description"] = info.description;

    // MCP 协议要求 inputSchema.type == "object"。
    // 兜底：若上游（builtin 工具 / SDK 自定义工具）未填 type，自动补全。
    Dictionary schema = info.input_schema;
    if (!schema.has("type")) {
        schema["type"] = "object";
        if (!schema.has("properties")) {
            schema["properties"] = Dictionary();
        }
    }
    d["inputSchema"] = schema;
    return d;
}

Array HandlerRegistry::get_all_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        result.push_back(make_tool_entry(kv.value));
    }
    return result;
}

Array HandlerRegistry::get_enabled_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (kv.value.enabled) {
            result.push_back(make_tool_entry(kv.value));
        }
    }
    return result;
}

bool HandlerRegistry::is_tool_enabled(const String &name) const {
    auto it = tool_info_.find(name);
    return it != tool_info_.end() && it->value.enabled;
}

void HandlerRegistry::set_tool_enabled(const String &name, bool enabled) {
    auto it = tool_info_.find(name);
    if (it != tool_info_.end()) {
        it->value.enabled = enabled;
    }
}

const CommandFn *HandlerRegistry::find(const String &name) const {
    auto it = table_.find(name);
    if (it == table_.end()) return nullptr;
    return &it->value;
}

bool HandlerRegistry::has(const String &name) const { return table_.has(name); }
int HandlerRegistry::size() const { return (int)table_.size(); }

// ---------------------------------------------------------------------------
// Category queries (for progressive disclosure)
// ---------------------------------------------------------------------------

Array HandlerRegistry::get_categories() const {
    // Collect unique categories with tool count and description
    HashMap<String, int> counts;
    HashMap<String, String> descriptions;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        const String &cat = kv.value.category;
        if (cat.is_empty()) continue;
        counts[cat] = counts.has(cat) ? counts[cat] + 1 : 1;
        // Prefer category_description; fall back to first tool's brief
        if (!kv.value.category_description.is_empty()) {
            descriptions[cat] = kv.value.category_description;
        } else if (!descriptions.has(cat)) {
            descriptions[cat] = kv.value.brief;
        }
    }

    Array result;
    for (const KeyValue<String, int> &kv : counts) {
        Dictionary d;
        d["name"] = kv.key;
        d["tool_count"] = kv.value;
        if (descriptions.has(kv.key)) {
            d["description"] = descriptions[kv.key];
        }
        result.push_back(d);
    }
    return result;
}

Array HandlerRegistry::get_tools_in_category(const String &category) const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        if (kv.value.category != category) continue;
        Dictionary d;
        d["name"] = kv.value.name;
        d["brief"] = kv.value.brief;
        result.push_back(d);
    }
    return result;
}

const ToolInfo *HandlerRegistry::get_tool_schema(const String &name) const {
    return find_tool_info(name);
}

// ---------------------------------------------------------------------------
// Always-on tools (meta-tools + godot_info)
// ---------------------------------------------------------------------------

Array HandlerRegistry::get_always_on_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.enabled) continue;
        if (kv.value.is_meta) {
            result.push_back(make_tool_entry(kv.value));
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Counts
// ---------------------------------------------------------------------------

int HandlerRegistry::builtin_tool_count() const {
    int count = 0;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (!kv.value.is_custom) ++count;
    }
    return count;
}

int HandlerRegistry::custom_tool_count() const {
    int count = 0;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (kv.value.is_custom) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Built-in tool registration — delegates to codegen'd register_itools()
// ---------------------------------------------------------------------------

void register_itools(HandlerRegistry &reg);

void register_all_tools(HandlerRegistry &reg) {
    register_itools(reg);
}

}  // namespace godot_mcp
