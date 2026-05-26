// =====================================================================
// commands/handler_registry.cpp — Registry implementation + entry point.
//
// register_all_tools() is the single function the rest of the code calls
// to populate the table. It delegates to per-group registration helpers
// implemented in each cmd_*.cpp file. New tool groups are added by:
//   1. Declaring `void register_<group>(HandlerRegistry &)` below.
//   2. Implementing it in the corresponding cmd_*.cpp.
//   3. Calling it from register_all_tools().
// =====================================================================

#include "handler_registry.hpp"

namespace godot_mcp {

HandlerRegistry::HandlerRegistry() = default;

void HandlerRegistry::register_tool(const godot::String &name, CommandFn fn) {
    table_[name] = std::move(fn);
}

const CommandFn *HandlerRegistry::find(const godot::String &name) const {
    auto it = table_.find(name);
    if (it == table_.end()) {
        return nullptr;
    }
    return &it->value;
}

bool HandlerRegistry::has(const godot::String &name) const {
    return table_.has(name);
}

int HandlerRegistry::size() const {
    return (int)table_.size();
}

// Forward declarations of per-group registrators. Each cmd_*.cpp file
// implements one of these so the registry stays a one-line addition.
void register_meta(HandlerRegistry &reg);
// TODO: Following groups will be added in subsequent steps:
//   void register_node(HandlerRegistry &reg);
//   void register_property(HandlerRegistry &reg);
//   void register_property_3d(HandlerRegistry &reg);
//   void register_collision(HandlerRegistry &reg);
//   void register_find(HandlerRegistry &reg);
//   void register_scene(HandlerRegistry &reg);
//   void register_script_gd(HandlerRegistry &reg);
//   void register_script_cs(HandlerRegistry &reg);
//   void register_script_helpers(HandlerRegistry &reg);
//   void register_project_settings(HandlerRegistry &reg);
//   void register_editor_control(HandlerRegistry &reg);
//   void register_input_map(HandlerRegistry &reg);
//   void register_plugin_management(HandlerRegistry &reg);
//   void register_search(HandlerRegistry &reg);
//   void register_undo(HandlerRegistry &reg);

void register_all_tools(HandlerRegistry &reg) {
    register_meta(reg);
}

}  // namespace godot_mcp
