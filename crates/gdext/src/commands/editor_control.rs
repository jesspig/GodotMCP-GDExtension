use serde_json::{Value, json};

use godot::classes::EditorInterface;
use godot::classes::Engine;
use godot::classes::ProjectSettings;
use godot::obj::Singleton;
use godot::prelude::GString;

use super::{CommandHandler, pipe};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &[
    "play_current_scene",
    "play_main_scene",
    "stop_scene",
    "is_scene_playing",
    "refresh_filesystem",
    "get_editor_info",
];

pub struct EditorControlCommands;

impl EditorControlCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for EditorControlCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for EditorControlCommands {
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
        Box::pin(self.handle_editor_control_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "editor_control"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl EditorControlCommands {
    pub async fn handle_editor_control_tool(
        &self,
        tool: &str,
        _args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        match tool {
            "play_current_scene" => pipe(d.submit(cmd_play_current_scene).await),
            "play_main_scene" => pipe(d.submit(cmd_play_main_scene).await),
            "stop_scene" => pipe(d.submit(cmd_stop_scene).await),
            "is_scene_playing" => pipe(d.submit(cmd_is_scene_playing).await),
            "refresh_filesystem" => pipe(d.submit(cmd_refresh_filesystem).await),
            "get_editor_info" => pipe(d.submit(cmd_get_editor_info).await),
            _ => Err(format!("Unknown editor_control tool: {}", tool)),
        }
    }
}

fn cmd_play_current_scene() -> Value {
    let mut ei = EditorInterface::singleton();
    ei.play_current_scene();
    json!({"playing": true})
}

fn cmd_play_main_scene() -> Value {
    let mut ei = EditorInterface::singleton();
    let main_scene = ProjectSettings::singleton()
        .get_setting("application/run/main_scene")
        .to::<GString>()
        .to_string();
    ei.play_main_scene();
    json!({"playing": true, "main_scene": main_scene})
}

fn cmd_stop_scene() -> Value {
    let mut ei = EditorInterface::singleton();
    ei.stop_playing_scene();
    json!({"stopped": true})
}

fn cmd_is_scene_playing() -> Value {
    let ei = EditorInterface::singleton();
    let playing = ei.is_playing_scene();
    let scene = if playing {
        let s = ei.get_playing_scene().to_string();
        if s.is_empty() { Value::Null } else { json!(s) }
    } else {
        Value::Null
    };
    json!({"playing": playing, "scene_path": scene})
}

fn cmd_refresh_filesystem() -> Value {
    let ei = EditorInterface::singleton();
    if let Some(mut efs) = ei.get_resource_filesystem() {
        efs.scan();
        json!({"scanning": true})
    } else {
        json!({"error": "EditorFileSystem not available"})
    }
}

fn dict_get_i64(
    dict: &godot::builtin::Dictionary<godot::prelude::Variant, godot::prelude::Variant>,
    key: &str,
) -> i64 {
    dict.get(key)
        .map(|v| {
            if let Ok(i) = v.try_to::<i64>() {
                i
            } else if let Ok(f) = v.try_to::<f64>() {
                f as i64
            } else {
                0
            }
        })
        .unwrap_or(0)
}

fn dict_get_string(
    dict: &godot::builtin::Dictionary<godot::prelude::Variant, godot::prelude::Variant>,
    key: &str,
) -> String {
    dict.get(key)
        .map(|v| v.to::<GString>().to_string())
        .unwrap_or_default()
}

fn cmd_get_editor_info() -> Value {
    let ei = EditorInterface::singleton();

    let engine = Engine::singleton();
    let version_info = engine.get_version_info();
    let engine_version = format!(
        "{}.{}.{}{}",
        dict_get_i64(&version_info, "major"),
        dict_get_i64(&version_info, "minor"),
        dict_get_i64(&version_info, "patch"),
        {
            let status = dict_get_string(&version_info, "status");
            if status == "stable" || status.is_empty() {
                String::new()
            } else {
                format!("-{}", status)
            }
        }
    );

    let editor_scale = ei.get_editor_scale();

    let editor_language = ei
        .get_editor_settings()
        .map(|s| {
            s.get_setting(&GString::from("interface/editor/editor_language"))
                .to::<GString>()
                .to_string()
        })
        .unwrap_or_else(|| "en".to_string());

    let data_dir = ei
        .get_editor_paths()
        .map(|p| p.get_data_dir().to_string())
        .unwrap_or_default();
    let config_dir = ei
        .get_editor_paths()
        .map(|p| p.get_config_dir().to_string())
        .unwrap_or_default();
    let cache_dir = ei
        .get_editor_paths()
        .map(|p| p.get_cache_dir().to_string())
        .unwrap_or_default();
    let project_settings_dir = ei
        .get_editor_paths()
        .map(|p| p.get_project_settings_dir().to_string())
        .unwrap_or_default();

    json!({
        "engine_version": engine_version,
        "editor_scale": editor_scale,
        "editor_language": editor_language,
        "editor_paths": {
            "data_dir": data_dir,
            "config_dir": config_dir,
            "cache_dir": cache_dir,
            "project_settings_dir": project_settings_dir,
        }
    })
}
