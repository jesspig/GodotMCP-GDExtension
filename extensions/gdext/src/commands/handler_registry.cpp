// =====================================================================
// commands/handler_registry.cpp — Registry implementation + entry point
// =====================================================================

#include "handler_registry.hpp"

namespace godot_mcp {

HandlerRegistry::HandlerRegistry() = default;

void HandlerRegistry::register_tool(const godot::String &name, CommandFn fn) {
    table_[name] = std::move(fn);
}

const CommandFn *HandlerRegistry::find(const godot::String &name) const {
    auto it = table_.find(name);
    if (it == table_.end()) return nullptr;
    return &it->value;
}

bool HandlerRegistry::has(const godot::String &name) const { return table_.has(name); }
int HandlerRegistry::size() const { return (int)table_.size(); }

// All 16 command group registrators
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