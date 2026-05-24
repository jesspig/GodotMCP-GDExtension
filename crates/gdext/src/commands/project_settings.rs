use serde_json::{Value, json};

use godot::classes::ProjectSettings;
use godot::obj::Singleton;
use godot::prelude::{GString, Variant};

use super::{j2v, pipe, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "get_project_setting",
    "set_project_setting",
    "set_main_scene",
];

pub struct ProjectSettingsCommands;

impl ProjectSettingsCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for ProjectSettingsCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for ProjectSettingsCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("ProjectSettingsCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "project_settings"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl ProjectSettingsCommands {
    pub async fn handle_project_settings_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "get_project_setting" => pipe(d.submit(move || cmd_get_project_setting(&a)).await),
            "set_project_setting" => pipe(d.submit(move || cmd_set_project_setting(&a)).await),
            "set_main_scene" => pipe(d.submit(move || cmd_set_main_scene(&a)).await),
            _ => Err(format!("Unknown project_settings tool: {}", tool)),
        }
    }
}

fn cmd_get_project_setting(args: &Value) -> Value {
    let prop = s(args, "property");
    if prop.is_empty() {
        return json!({"error": "missing 'property'"});
    }
    let ps = ProjectSettings::singleton();
    let val = ps.get_setting(&GString::from(prop.as_str()));
    json!({"property": prop, "value": v2j(&val)})
}

fn cmd_set_project_setting(args: &Value) -> Value {
    let prop = s(args, "property");
    let val = match args.get("value") {
        Some(v) => v.clone(),
        None => return json!({"error": "missing 'value'"}),
    };
    if prop.is_empty() {
        return json!({"error": "missing 'property'"});
    }
    let mut ps = ProjectSettings::singleton();
    ps.set_setting(&GString::from(prop.as_str()), &j2v(&val));
    let mut result = json!({"property": prop, "value": val});
    if ps.save() != godot::global::Error::OK {
        result.as_object_mut().unwrap().insert(
            "warning".into(),
            json!("setting applied in memory but failed to save to disk"),
        );
    }
    result
}

fn cmd_set_main_scene(args: &Value) -> Value {
    let scene_path = s(args, "scene_path");
    if scene_path.is_empty() {
        return json!({"error": "missing 'scene_path'"});
    }
    let mut ps = ProjectSettings::singleton();
    ps.set_setting(
        &GString::from("application/run/main_scene"),
        &Variant::from(GString::from(scene_path.as_str())),
    );
    let mut result = json!({"main_scene": scene_path});
    if ps.save() != godot::global::Error::OK {
        result.as_object_mut().unwrap().insert(
            "warning".into(),
            json!("setting applied in memory but failed to save to disk"),
        );
    }
    result
}
