use serde_json::{Value, json};

use godot::classes::{ConfigFile, DirAccess, EditorInterface};
use godot::obj::{NewGd, Singleton};
use godot::prelude::GString;

use super::{CommandHandler, pipe, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &["list_plugins", "set_plugin_enabled"];

pub struct PluginManagementCommands;

impl PluginManagementCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for PluginManagementCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for PluginManagementCommands {
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
        Box::pin(self.handle_plugin_management_tool(tool, args, d))
    }
    fn group_name(&self) -> &str {
        "plugin_management"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl PluginManagementCommands {
    pub async fn handle_plugin_management_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "list_plugins" => pipe(d.submit(cmd_list_plugins).await),
            "set_plugin_enabled" => pipe(d.submit(move || cmd_set_plugin_enabled(&a)).await),
            _ => Err(format!("Unknown plugin_management tool: {}", tool)),
        }
    }
}

fn cfg_get(cf: &godot::obj::Gd<ConfigFile>, key: &str) -> String {
    cf.get_value(&GString::from("plugin"), &GString::from(key))
        .to::<GString>()
        .to_string()
}

fn cmd_list_plugins() -> Value {
    let Some(mut dir) = DirAccess::open(&GString::from("res://addons/")) else {
        return json!({"plugins": [], "count": 0, "error": "res://addons/ not found"});
    };
    let ei = EditorInterface::singleton();
    let mut plugins = Vec::new();
    dir.list_dir_begin();
    loop {
        let name = dir.get_next().to_string();
        if name.is_empty() {
            break;
        }
        if name == "." || name == ".." || !dir.current_is_dir() {
            continue;
        }
        let cfg_path = format!("res://addons/{}/plugin.cfg", name);
        if !godot::classes::FileAccess::file_exists(&GString::from(&cfg_path)) {
            continue;
        }
        let mut cf = ConfigFile::new_gd();
        if cf.load(&GString::from(&cfg_path)) != godot::global::Error::OK {
            continue;
        }
        let plugin_name = cfg_get(&cf, "name");
        let desc = cfg_get(&cf, "description");
        let author = cfg_get(&cf, "author");
        let ver = cfg_get(&cf, "version");
        let script = cfg_get(&cf, "script");
        let enabled = ei.is_plugin_enabled(&GString::from(&plugin_name));
        plugins.push(json!({
            "name": plugin_name,
            "description": desc,
            "author": author,
            "version": ver,
            "script": script,
            "enabled": enabled,
        }));
    }
    json!({"plugins": plugins, "count": plugins.len()})
}

fn cmd_set_plugin_enabled(args: &Value) -> Value {
    let plugin = {
        let p = s(args, "plugin");
        if p.is_empty() {
            return json!({"error": "missing 'plugin'"});
        }
        p
    };
    let enabled = args["enabled"].as_bool().unwrap_or(true);
    let mut ei = EditorInterface::singleton();
    let gs = GString::from(plugin.as_str());
    let was_enabled = ei.is_plugin_enabled(&gs);
    ei.set_plugin_enabled(&gs, enabled);
    json!({
        "plugin": plugin,
        "enabled": enabled,
        "previous": was_enabled,
    })
}
