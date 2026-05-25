use serde_json::{Value, json};

use godot::classes::{DirAccess, ProjectSettings};
use godot::obj::Singleton;
use godot::prelude::{GString, Variant};

use super::{j2v, pipe, s, v2j};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "get_project_setting",
    "set_project_setting",
    "set_main_scene",
    "list_autoloads",
    "add_autoload",
    "remove_autoload",
    "list_scenes",
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
    fn handle<'a>(
        &'a self,
        tool: &'a str,
        args: &'a Value,
        d: &'a MainThreadDispatcher,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Value, String>> + Send + 'a>>
    {
        Box::pin(self.handle_project_settings_tool(tool, args, d))
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
            "list_autoloads" => pipe(d.submit(cmd_list_autoloads).await),
            "add_autoload" => pipe(d.submit(move || cmd_add_autoload(&a)).await),
            "remove_autoload" => pipe(d.submit(move || cmd_remove_autoload(&a)).await),
            "list_scenes" => pipe(d.submit(move || cmd_list_scenes(&a)).await),
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

fn cmd_list_autoloads() -> Value {
    let ps = ProjectSettings::singleton();
    let mut autoloads = Vec::new();
    let prefix = "autoload/";
    let props = ps.get_property_list();
    for prop in props.iter_shared() {
        let name = prop.get_or_nil("name").to::<GString>().to_string();
        if let Some(autoload_name) = name.strip_prefix(prefix) {
            if autoload_name.is_empty() {
                continue;
            }
            let val = ps
                .get_setting(&GString::from(&name))
                .to::<GString>()
                .to_string();
            let (path, singleton) = if let Some(stripped) = val.strip_prefix('*') {
                (stripped.to_string(), true)
            } else {
                (val, false)
            };
            autoloads.push(json!({
                "name": autoload_name,
                "path": path,
                "singleton": singleton,
            }));
        }
    }
    json!({"autoloads": autoloads, "count": autoloads.len()})
}

fn cmd_add_autoload(args: &Value) -> Value {
    let name = s(args, "name");
    let path = s(args, "path");
    if name.is_empty() || path.is_empty() {
        return json!({"error": "missing 'name' or 'path'"});
    }
    let singleton = args["singleton"].as_bool().unwrap_or(true);
    let value = if singleton {
        format!("*{}", path)
    } else {
        path.clone()
    };
    let mut ps = ProjectSettings::singleton();
    let key = format!("autoload/{}", name);
    ps.set_setting(&GString::from(&key), &Variant::from(GString::from(&value)));
    let mut result = json!({"name": name, "path": path, "singleton": singleton});
    if ps.save() != godot::global::Error::OK {
        result.as_object_mut().unwrap().insert(
            "warning".into(),
            json!("setting applied in memory but failed to save to disk"),
        );
    }
    result
}

fn cmd_remove_autoload(args: &Value) -> Value {
    let name = s(args, "name");
    if name.is_empty() {
        return json!({"error": "missing 'name'"});
    }
    let mut ps = ProjectSettings::singleton();
    let key = format!("autoload/{}", name);
    ps.set_setting(&GString::from(&key), &Variant::nil());
    let mut result = json!({"name": name, "removed": true});
    if ps.save() != godot::global::Error::OK {
        result.as_object_mut().unwrap().insert(
            "warning".into(),
            json!("setting applied in memory but failed to save to disk"),
        );
    }
    result
}

fn cmd_list_scenes(args: &Value) -> Value {
    let root = {
        let r = s(args, "root");
        if r.is_empty() {
            "res://".to_string()
        } else {
            r
        }
    };
    let max_results = args["max_results"].as_u64().unwrap_or(1000) as usize;

    let mut files = Vec::new();
    list_scenes_recursive(&root, max_results, &mut files);

    let truncated = files.len() >= max_results;
    let scenes: Vec<Value> = files.iter().map(|f| json!({"path": f})).collect();
    json!({"scenes": scenes, "count": scenes.len(), "truncated": truncated})
}

fn list_scenes_recursive(dir: &str, max: usize, out: &mut Vec<String>) {
    if out.len() >= max {
        return;
    }
    let Some(mut d) = DirAccess::open(&GString::from(dir)) else {
        return;
    };
    d.list_dir_begin();
    loop {
        let name = d.get_next().to_string();
        if name.is_empty() {
            break;
        }
        if name == "." || name == ".." {
            continue;
        }
        let full = if dir == "res://" {
            format!("res://{}", name)
        } else {
            format!("{}/{}", dir, name)
        };
        if d.current_is_dir() {
            if name == ".godot" || name == ".import" {
                continue;
            }
            list_scenes_recursive(&full, max, out);
        } else if (name.ends_with(".tscn") || name.ends_with(".scn")) && out.len() < max {
            out.push(full);
        }
    }
}
