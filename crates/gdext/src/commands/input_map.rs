use serde_json::{Value, json};

use godot::classes::InputMap;
use godot::classes::ProjectSettings;
use godot::obj::{EngineEnum, NewGd, Singleton};
use godot::prelude::GString;

use super::{CommandHandler, pipe, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "list_input_actions",
    "add_input_action",
    "set_input_action_events",
    "remove_input_action",
];

pub struct InputMapCommands;

impl InputMapCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for InputMapCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for InputMapCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("InputMapCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "input_map"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl InputMapCommands {
    pub async fn handle_input_map_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "list_input_actions" => pipe(d.submit(move || cmd_list_input_actions(&a)).await),
            "add_input_action" => pipe(d.submit(move || cmd_add_input_action(&a)).await),
            "set_input_action_events" => {
                pipe(d.submit(move || cmd_set_input_action_events(&a)).await)
            }
            "remove_input_action" => pipe(d.submit(move || cmd_remove_input_action(&a)).await),
            _ => Err(format!("Unknown input_map tool: {}", tool)),
        }
    }
}

fn serialize_event(event: &godot::prelude::Gd<godot::classes::InputEvent>) -> Value {
    use godot::classes::{
        InputEventJoypadButton, InputEventJoypadMotion, InputEventKey, InputEventMouseButton,
    };

    if let Ok(key) = event.clone().try_cast::<InputEventKey>() {
        return json!({
            "type": "key",
            "keycode": key.get_keycode().ord(),
            "physical_keycode": key.get_physical_keycode().ord(),
            "ctrl": key.is_ctrl_pressed(),
            "shift": key.is_shift_pressed(),
            "alt": key.is_alt_pressed(),
            "meta": key.is_meta_pressed(),
            "pressed": key.is_pressed(),
        });
    }
    if let Ok(mb) = event.clone().try_cast::<InputEventMouseButton>() {
        return json!({
            "type": "mouse_button",
            "button_index": mb.get_button_index().ord(),
            "double_click": mb.is_double_click(),
            "pressed": mb.is_pressed(),
        });
    }
    if let Ok(jb) = event.clone().try_cast::<InputEventJoypadButton>() {
        return json!({
            "type": "joypad_button",
            "device": jb.get_device(),
            "button_index": jb.get_button_index().ord(),
            "pressed": jb.is_pressed(),
        });
    }
    if let Ok(jm) = event.clone().try_cast::<InputEventJoypadMotion>() {
        return json!({
            "type": "joypad_motion",
            "device": jm.get_device(),
            "axis": jm.get_axis().ord(),
            "axis_value": jm.get_axis_value(),
        });
    }
    json!({"type": "unknown"})
}

fn deserialize_event(val: &Value) -> Option<godot::prelude::Gd<godot::classes::InputEvent>> {
    use godot::classes::{
        InputEventJoypadButton, InputEventJoypadMotion, InputEventKey, InputEventMouseButton,
    };
    use godot::global::{JoyAxis, JoyButton, Key, MouseButton};

    let event_type = val.get("type")?.as_str()?;
    match event_type {
        "key" => {
            let mut ev = InputEventKey::new_gd();
            if let Some(kc) = val.get("keycode").and_then(|v| v.as_i64()) {
                ev.set_keycode(Key::try_from_ord(kc as i32).unwrap_or(Key::NONE));
            }
            if let Some(pk) = val.get("physical_keycode").and_then(|v| v.as_i64()) {
                ev.set_physical_keycode(Key::try_from_ord(pk as i32).unwrap_or(Key::NONE));
            }
            if let Some(c) = val.get("ctrl").and_then(|v| v.as_bool()) {
                ev.set_ctrl_pressed(c);
            }
            if let Some(sh) = val.get("shift").and_then(|v| v.as_bool()) {
                ev.set_shift_pressed(sh);
            }
            if let Some(a) = val.get("alt").and_then(|v| v.as_bool()) {
                ev.set_alt_pressed(a);
            }
            if let Some(m) = val.get("meta").and_then(|v| v.as_bool()) {
                ev.set_meta_pressed(m);
            }
            if let Some(p) = val.get("pressed").and_then(|v| v.as_bool()) {
                ev.set_pressed(p);
            }
            Some(ev.upcast())
        }
        "mouse_button" => {
            let mut ev = InputEventMouseButton::new_gd();
            if let Some(idx) = val.get("button_index").and_then(|v| v.as_i64()) {
                ev.set_button_index(
                    MouseButton::try_from_ord(idx as i32).unwrap_or(MouseButton::NONE),
                );
            }
            if let Some(dc) = val.get("double_click").and_then(|v| v.as_bool()) {
                ev.set_double_click(dc);
            }
            if let Some(p) = val.get("pressed").and_then(|v| v.as_bool()) {
                ev.set_pressed(p);
            }
            Some(ev.upcast())
        }
        "joypad_button" => {
            let mut ev = InputEventJoypadButton::new_gd();
            if let Some(d) = val.get("device").and_then(|v| v.as_i64()) {
                ev.set_device(d as i32);
            }
            if let Some(idx) = val.get("button_index").and_then(|v| v.as_i64()) {
                ev.set_button_index(
                    JoyButton::try_from_ord(idx as i32).unwrap_or(JoyButton::INVALID),
                );
            }
            if let Some(p) = val.get("pressed").and_then(|v| v.as_bool()) {
                ev.set_pressed(p);
            }
            Some(ev.upcast())
        }
        "joypad_motion" => {
            let mut ev = InputEventJoypadMotion::new_gd();
            if let Some(d) = val.get("device").and_then(|v| v.as_i64()) {
                ev.set_device(d as i32);
            }
            if let Some(axis) = val.get("axis").and_then(|v| v.as_i64()) {
                ev.set_axis(JoyAxis::try_from_ord(axis as i32).unwrap_or(JoyAxis::INVALID));
            }
            if let Some(av) = val.get("axis_value").and_then(|v| v.as_f64()) {
                ev.set_axis_value(av as f32);
            }
            Some(ev.upcast())
        }
        _ => None,
    }
}

fn is_editor_builtin_action(name: &str) -> bool {
    name.starts_with("ui_")
        || name.starts_with("spatial_editor/")
        || name.starts_with("canvas_editor/")
        || name.starts_with("editor/")
        || name.starts_with("text_editor/")
        || name.starts_with("script_editor/")
}

fn cmd_list_input_actions(args: &Value) -> Value {
    let include_builtin = args["include_builtin"].as_bool().unwrap_or(false);
    let mut im = InputMap::singleton();
    let actions = im.get_actions();
    let mut result = Vec::new();
    for action in actions.iter_shared() {
        let name = GString::from(&action).to_string();
        if !include_builtin && is_editor_builtin_action(&name) {
            continue;
        }
        let sn = godot::prelude::StringName::from(&name);
        let deadzone = im.action_get_deadzone(&sn);
        let events = im.action_get_events(&sn);
        let event_list: Vec<Value> = events.iter_shared().map(|e| serialize_event(&e)).collect();
        result.push(json!({
            "name": name,
            "deadzone": deadzone,
            "events": event_list,
        }));
    }
    json!({"actions": result, "count": result.len()})
}

fn cmd_add_input_action(args: &Value) -> Value {
    let name = s(args, "name");
    if name.is_empty() {
        return json!({"error": "missing 'name'"});
    }
    let deadzone = args["deadzone"].as_f64().unwrap_or(0.5) as f32;
    let mut im = InputMap::singleton();
    let sn = godot::prelude::StringName::from(name.as_str());
    if im.has_action(&sn) {
        return json!({"error": format!("action '{}' already exists", name)});
    }
    im.add_action(&sn);
    im.action_set_deadzone(&sn, deadzone);
    let mut ps = ProjectSettings::singleton();
    if ps.save() != godot::global::Error::OK {
        return json!({"name": name, "deadzone": deadzone, "warning": "saved to memory but disk write failed"});
    }
    json!({"name": name, "deadzone": deadzone})
}

fn cmd_set_input_action_events(args: &Value) -> Value {
    let name = s(args, "name");
    if name.is_empty() {
        return json!({"error": "missing 'name'"});
    }
    let mode = s(args, "mode");
    let mode = if mode.is_empty() { "replace" } else { &mode };
    let events = match args.get("events") {
        Some(Value::Array(arr)) => arr,
        _ => {
            if mode == "clear" {
                &vec![]
            } else {
                return json!({"error": "missing 'events' array"});
            }
        }
    };

    let mut im = InputMap::singleton();
    let sn = godot::prelude::StringName::from(name.as_str());
    if !im.has_action(&sn) {
        return json!({"error": format!("action '{}' not found", name)});
    }

    match mode {
        "clear" => {
            im.action_erase_events(&sn);
        }
        "replace" => {
            im.action_erase_events(&sn);
            for ev in events {
                if let Some(event) = deserialize_event(ev) {
                    im.action_add_event(&sn, &event);
                }
            }
        }
        "add" => {
            for ev in events {
                if let Some(event) = deserialize_event(ev) {
                    im.action_add_event(&sn, &event);
                }
            }
        }
        _ => return json!({"error": "mode must be 'replace', 'add', or 'clear'"}),
    }

    let mut ps = ProjectSettings::singleton();
    if ps.save() != godot::global::Error::OK {
        return json!({"name": name, "events_count": events.len(), "warning": "saved to memory but disk write failed"});
    }
    json!({"name": name, "events_count": events.len()})
}

fn cmd_remove_input_action(args: &Value) -> Value {
    let name = s(args, "name");
    if name.is_empty() {
        return json!({"error": "missing 'name'"});
    }
    let mut im = InputMap::singleton();
    let sn = godot::prelude::StringName::from(name.as_str());
    if !im.has_action(&sn) {
        return json!({"error": format!("action '{}' not found", name)});
    }
    im.erase_action(&sn);
    let mut ps = ProjectSettings::singleton();
    if ps.save() != godot::global::Error::OK {
        return json!({"name": name, "removed": true, "warning": "saved to memory but disk write failed"});
    }
    json!({"name": name, "removed": true})
}
