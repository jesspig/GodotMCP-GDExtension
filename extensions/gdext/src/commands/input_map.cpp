// commands/input_map.cpp — input action tools

#include "cmd_utils.hpp"
#include "handler_registry.hpp"
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
using namespace godot;

namespace godot_mcp {
namespace {

Dictionary serialize_event(Ref<InputEvent> ev) {
    Dictionary r;
    if (Object::cast_to<InputEventKey>(ev.ptr())) {
        Ref<InputEventKey> k = ev; r["type"] = "key"; r["keycode"] = (int64_t)k->get_keycode(); r["physical_keycode"] = (int64_t)k->get_physical_keycode();
        r["ctrl"] = k->is_ctrl_pressed(); r["shift"] = k->is_shift_pressed(); r["alt"] = k->is_alt_pressed(); r["meta"] = k->is_meta_pressed(); r["pressed"] = k->is_pressed();
    } else if (Object::cast_to<InputEventMouseButton>(ev.ptr())) {
        Ref<InputEventMouseButton> mb = ev; r["type"] = "mouse_button"; r["button_index"] = (int64_t)mb->get_button_index(); r["double_click"] = mb->is_double_click(); r["pressed"] = mb->is_pressed();
    } else if (Object::cast_to<InputEventJoypadButton>(ev.ptr())) {
        Ref<InputEventJoypadButton> jb = ev; r["type"] = "joypad_button"; r["device"] = (int64_t)jb->get_device(); r["button_index"] = (int64_t)jb->get_button_index(); r["pressed"] = jb->is_pressed();
    } else if (Object::cast_to<InputEventJoypadMotion>(ev.ptr())) {
        Ref<InputEventJoypadMotion> jm = ev; r["type"] = "joypad_motion"; r["device"] = (int64_t)jm->get_device(); r["axis"] = (int64_t)jm->get_axis(); r["axis_value"] = jm->get_axis_value();
    }
    return r;
}
Variant deserialize_event(const Dictionary &d) {
    String t = args_string(d, "type");
    if (t == "key") {
        Ref<InputEventKey> ev; ev.instantiate();
        if (d.has("keycode")) ev->set_keycode((Key)(int64_t)d["keycode"]);
        if (d.has("physical_keycode")) ev->set_physical_keycode((Key)(int64_t)d["physical_keycode"]);
        if (d.has("ctrl")) ev->set_ctrl_pressed(d["ctrl"]); if (d.has("shift")) ev->set_shift_pressed(d["shift"]);
        if (d.has("alt")) ev->set_alt_pressed(d["alt"]); if (d.has("meta")) ev->set_meta_pressed(d["meta"]);
        if (d.has("pressed")) ev->set_pressed(d["pressed"]);
        return ev;
    } else if (t == "mouse_button") {
        Ref<InputEventMouseButton> ev; ev.instantiate();
        if (d.has("button_index")) ev->set_button_index((MouseButton)(int64_t)d["button_index"]);
        if (d.has("double_click")) ev->set_double_click(d["double_click"]); if (d.has("pressed")) ev->set_pressed(d["pressed"]);
        return ev;
    } else if (t == "joypad_button") {
        Ref<InputEventJoypadButton> ev; ev.instantiate();
        if (d.has("device")) ev->set_device((int64_t)d["device"]);
        if (d.has("button_index")) ev->set_button_index((JoyButton)(int64_t)d["button_index"]);
        if (d.has("pressed")) ev->set_pressed(d["pressed"]);
        return ev;
    } else if (t == "joypad_motion") {
        Ref<InputEventJoypadMotion> ev; ev.instantiate();
        if (d.has("device")) ev->set_device((int64_t)d["device"]);
        if (d.has("axis")) ev->set_axis((JoyAxis)(int64_t)d["axis"]);
        if (d.has("axis_value")) ev->set_axis_value((float)(double)d["axis_value"]);
        return ev;
    }
    return Variant();
}

Dictionary cmd_list_input_actions(const Dictionary &a) {
    bool builtin = args_bool(a, "include_builtin", false);
    InputMap *im = InputMap::get_singleton(); im->load_from_project_settings();
    Array acts = im->get_actions();
    Array out;
    for (int i = 0; i < acts.size(); i++) {
        String name = (String)acts[i];
        if (!builtin && (name.begins_with("ui_") || name.begins_with("editor/") || name.begins_with("spatial_editor/") || name.begins_with("canvas_editor/") || name.begins_with("text_editor/") || name.begins_with("script_editor/"))) continue;
        float dz = im->action_get_deadzone(name);
        Array evs_arr = im->action_get_events(name);
        Array evs_json;
        for (int j = 0; j < evs_arr.size(); j++) { Ref<InputEvent> ev = evs_arr[j]; evs_json.append(serialize_event(ev)); }
        Dictionary e; e["name"] = name; e["deadzone"] = dz; e["events"] = evs_json;
        out.append(e);
    }
    Dictionary r; r["actions"] = out; r["count"] = (int64_t)out.size(); return r;
}
Dictionary cmd_add_input_action(const Dictionary &a) {
    String name = args_string(a, "name"); if (name.is_empty()) return make_error("missing 'name'");
    float dz = args_float(a, "deadzone", 0.5);
    String key = "input/" + name;
    if (ProjectSettings::get_singleton()->has_setting(key)) return make_error("action '" + name + "' already exists");
    Dictionary ad; ad["deadzone"] = dz; ad["events"] = Array();
    ProjectSettings::get_singleton()->set_setting(key, ad);
    Dictionary r; r["name"] = name; r["deadzone"] = dz;
    if (ProjectSettings::get_singleton()->save() != OK) r["warning"] = "disk write failed";
    InputMap::get_singleton()->load_from_project_settings();
    return r;
}
Dictionary cmd_set_input_action_events(const Dictionary &a) {
    String name = args_string(a, "name"); if (name.is_empty()) return make_error("missing 'name'");
    String mode = args_string(a, "mode", "replace");
    String key = "input/" + name;
    if (!ProjectSettings::get_singleton()->has_setting(key)) return make_error("action '" + name + "' not found");
    Variant existing = ProjectSettings::get_singleton()->get_setting(key);
    Dictionary action_dict = existing;
    if (mode == "clear") { action_dict["events"] = Array(); }
    else if (mode == "replace" || mode == "add") {
        Array new_events;
        if (mode == "add" && action_dict.has("events")) { Array old = action_dict["events"]; for (int i = 0; i < old.size(); i++) new_events.append(old[i]); }
        if (a.has("events") && a["events"].get_type() == Variant::ARRAY) {
            Array evs = a["events"];
            for (int i = 0; i < evs.size(); i++) {
                Dictionary ed = evs[i];
                Variant ev = deserialize_event(ed);
                if (ev.get_type() != Variant::NIL) new_events.append(ev);
            }
        }
        action_dict["events"] = new_events;
    } else return make_error("mode must be 'replace', 'add', or 'clear'");
    ProjectSettings::get_singleton()->set_setting(key, action_dict);
    Dictionary r; r["name"] = name;
    if (ProjectSettings::get_singleton()->save() != OK) r["warning"] = "disk write failed";
    InputMap::get_singleton()->load_from_project_settings();
    return r;
}
Dictionary cmd_remove_input_action(const Dictionary &a) {
    String name = args_string(a, "name"); if (name.is_empty()) return make_error("missing 'name'");
    String key = "input/" + name;
    if (!ProjectSettings::get_singleton()->has_setting(key)) return make_error("action '" + name + "' not found");
    ProjectSettings::get_singleton()->set_setting(key, Variant());
    ProjectSettings::get_singleton()->save();
    InputMap::get_singleton()->load_from_project_settings();
    Dictionary r; r["name"] = name; r["removed"] = true; return r;
}
} // namespace

void register_input_map(HandlerRegistry &reg) {
    reg.register_tool("list_input_actions", cmd_list_input_actions);
    reg.register_tool("add_input_action", cmd_add_input_action);
    reg.register_tool("set_input_action_events", cmd_set_input_action_events);
    reg.register_tool("remove_input_action", cmd_remove_input_action);
}
} // namespace godot_mcp