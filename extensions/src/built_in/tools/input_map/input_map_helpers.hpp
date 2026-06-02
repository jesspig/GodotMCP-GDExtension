#pragma once

#include "built_in/cmd_utils.hpp"
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>

namespace godot_mcp {

inline Dictionary serialize_event(Ref<InputEvent> ev) {
    Dictionary r;
    if (Object::cast_to<InputEventKey>(ev.ptr())) {
        Ref<InputEventKey> k = ev; r["type"] = "key"; r["keycode"] = (int64_t)k->get_keycode();
        r["physical_keycode"] = (int64_t)k->get_physical_keycode();
        r["ctrl"] = k->is_ctrl_pressed(); r["shift"] = k->is_shift_pressed();
        r["alt"] = k->is_alt_pressed(); r["meta"] = k->is_meta_pressed(); r["pressed"] = k->is_pressed();
    } else if (Object::cast_to<InputEventMouseButton>(ev.ptr())) {
        Ref<InputEventMouseButton> mb = ev; r["type"] = "mouse_button";
        r["button_index"] = (int64_t)mb->get_button_index(); r["double_click"] = mb->is_double_click(); r["pressed"] = mb->is_pressed();
    } else if (Object::cast_to<InputEventJoypadButton>(ev.ptr())) {
        Ref<InputEventJoypadButton> jb = ev; r["type"] = "joypad_button";
        r["device"] = (int64_t)jb->get_device(); r["button_index"] = (int64_t)jb->get_button_index(); r["pressed"] = jb->is_pressed();
    } else if (Object::cast_to<InputEventJoypadMotion>(ev.ptr())) {
        Ref<InputEventJoypadMotion> jm = ev; r["type"] = "joypad_motion";
        r["device"] = (int64_t)jm->get_device(); r["axis"] = (int64_t)jm->get_axis(); r["axis_value"] = jm->get_axis_value();
    }
    return r;
}
inline Variant deserialize_event(const Dictionary &d) {
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

} // namespace godot_mcp
