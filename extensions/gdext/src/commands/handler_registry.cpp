#include "handler_registry.hpp"

#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace godot_mcp {

HandlerRegistry::HandlerRegistry() = default;

void HandlerRegistry::register_tool(const String &name, CommandFn fn) {
    table_[name] = std::move(fn);
}

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
        ToolInfo info;
        info.name = name;
        info.description = entry.get("description", "");
        info.input_schema = entry.get("input_schema", Dictionary());
        info.enabled = true;
        tool_info_[name] = info;
    }
}

const ToolInfo *HandlerRegistry::find_tool_info(const String &name) const {
    auto it = tool_info_.find(name);
    if (it == tool_info_.end()) return nullptr;
    return &it->value;
}

Array HandlerRegistry::get_all_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        Dictionary d;
        d["name"] = kv.value.name;
        d["description"] = kv.value.description;
        d["inputSchema"] = kv.value.input_schema;
        result.push_back(d);
    }
    return result;
}

Array HandlerRegistry::get_enabled_tools() const {
    Array result;
    for (const KeyValue<String, ToolInfo> &kv : tool_info_) {
        if (kv.value.enabled) {
            Dictionary d;
            d["name"] = kv.value.name;
            d["description"] = kv.value.description;
            d["inputSchema"] = kv.value.input_schema;
            result.push_back(d);
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

void register_meta(HandlerRegistry &reg);
void register_node(HandlerRegistry &reg);
void register_property(HandlerRegistry &reg);
void register_property_3d(HandlerRegistry &reg);
void register_collision(HandlerRegistry &reg);
void register_find(HandlerRegistry &reg);
void register_scene(HandlerRegistry &reg);
void register_script_gd(HandlerRegistry &reg);
void register_script_cs(HandlerRegistry &reg);
void register_script_helpers(HandlerRegistry &reg);
void register_project_settings(HandlerRegistry &reg);
void register_project_settings_ext(HandlerRegistry &reg);
void register_editor_control(HandlerRegistry &reg);
void register_input_map(HandlerRegistry &reg);
void register_plugin_management(HandlerRegistry &reg);
void register_undo(HandlerRegistry &reg);
void register_search(HandlerRegistry &reg);

void register_all_tools(HandlerRegistry &reg) {
    register_meta(reg);
    register_node(reg);
    register_property(reg);
    register_property_3d(reg);
    register_collision(reg);
    register_find(reg);
    register_scene(reg);
    register_script_gd(reg);
    register_script_cs(reg);
    register_script_helpers(reg);
    register_project_settings(reg);
    register_project_settings_ext(reg);
    register_editor_control(reg);
    register_input_map(reg);
    register_plugin_management(reg);
    register_undo(reg);
    register_search(reg);
}

}  // namespace godot_mcp
