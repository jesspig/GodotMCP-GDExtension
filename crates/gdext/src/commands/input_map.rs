use std::collections::HashSet;

use serde_json::{Value, json};

use godot::builtin::Variant;
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
    fn handle<'a>(
        &'a self,
        tool: &'a str,
        args: &'a Value,
        d: &'a MainThreadDispatcher,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Value, String>> + Send + 'a>>
    {
        Box::pin(self.handle_input_map_tool(tool, args, d))
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

    im.load_from_project_settings();

    let actions = im.get_actions();
    let mut result = Vec::new();
    let mut seen = HashSet::new();
    for action in actions.iter_shared() {
        let name = GString::from(&action).to_string();
        if !seen.insert(name.clone()) {
            continue;
        }
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
    let key = format!("input/{}", name);

    // Check if the action already exists in project settings
    let mut ps = ProjectSettings::singleton();
    if ps.has_setting(&key) {
        return json!({"error": format!("action '{}' already exists", name)});
    }

    // Build the action dictionary matching Godot's project.godot format
    let mut action_dict = godot::builtin::Dictionary::<Variant, Variant>::new();
    action_dict.set("deadzone", &Variant::from(deadzone));
    action_dict.set(
        "events",
        &Variant::from(godot::builtin::Array::<Variant>::new()),
    );

    ps.set_setting(key.as_str(), &Variant::from(action_dict));
    if ps.save() != godot::global::Error::OK {
        return json!({"name": name, "deadzone": deadzone, "warning": "disk write failed"});
    }

    // Reload InputMap so list_input_actions sees the change immediately
    let mut im = InputMap::singleton();
    im.load_from_project_settings();

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

    let key = format!("input/{}", name);
    let mut ps = ProjectSettings::singleton();
    if !ps.has_setting(key.as_str()) {
        return json!({"error": format!("action '{}' not found", name)});
    }

    // Read existing action dict from project settings
    let existing = ps.get_setting(key.as_str());
    let mut action_dict: godot::builtin::Dictionary<Variant, Variant> =
        existing.try_to().unwrap_or_default();

    match mode {
        "clear" => {
            action_dict.set(
                "events",
                &Variant::from(godot::builtin::Array::<Variant>::new()),
            );
        }
        "replace" | "add" => {
            let mut new_events = godot::builtin::Array::<Variant>::new();
            if mode == "add"
                && let Some(events_v) = action_dict.get("events")
                && let Ok(existing_events) = events_v.try_to::<godot::builtin::Array<Variant>>()
            {
                for e in existing_events.iter_shared() {
                    new_events.push(&e);
                }
            }
            for ev_json in events {
                if let Some(event_gd) = deserialize_event(ev_json) {
                    new_events.push(&Variant::from(event_gd));
                }
            }
            action_dict.set("events", &Variant::from(new_events));
        }
        _ => return json!({"error": "mode must be 'replace', 'add', or 'clear'"}),
    }

    ps.set_setting(key.as_str(), &Variant::from(action_dict));
    if ps.save() != godot::global::Error::OK {
        return json!({"name": name, "events_count": events.len(), "warning": "disk write failed"});
    }

    // Reload InputMap so the changes are visible immediately
    let mut im = InputMap::singleton();
    im.load_from_project_settings();

    json!({"name": name, "events_count": events.len()})
}

fn cmd_remove_input_action(args: &Value) -> Value {
    let name = s(args, "name");
    if name.is_empty() {
        return json!({"error": "missing 'name'"});
    }
    let key = format!("input/{}", name);
    let mut ps = ProjectSettings::singleton();
    if !ps.has_setting(key.as_str()) {
        return json!({"error": format!("action '{}' not found", name)});
    }
    // Clear the setting to remove it from project.godot
    ps.set_setting(key.as_str(), &Variant::nil());
    if ps.save() != godot::global::Error::OK {
        return json!({"name": name, "removed": true, "warning": "disk write failed"});
    }

    // Reload InputMap so list_input_actions reflects removal
    let mut im = InputMap::singleton();
    im.load_from_project_settings();

    json!({"name": name, "removed": true})
}
