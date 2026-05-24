use serde_json::Value;

use crate::commands::CommandHandler;
use crate::dispatcher::MainThreadDispatcher;

const TOOL_NAMES: &[&str] = &["ping", "get_engine_version", "get_plugin_version"];

pub struct MetaCommands {
    engine_version: String,
    plugin_version: String,
}

impl MetaCommands {
    pub fn new() -> Self {
        Self {
            engine_version: String::new(),
            plugin_version: env!("CARGO_PKG_VERSION").to_string(),
        }
    }

    pub fn with_engine_version(mut self, version: String) -> Self {
        self.engine_version = version;
        self
    }
}

impl Default for MetaCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl CommandHandler for MetaCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }

    fn execute(&self, _args: &Value, _dispatcher: &MainThreadDispatcher) -> Result<Value, String> {
        // MetaCommands tools don't need dispatcher since they only read PluginState.
        // The tool name is resolved by the caller routing, so this method is not called
        // directly for MetaCommands. Instead, handle_request resolves the tool and calls
        // the appropriate method. See handle_meta_tool below.
        Err("MetaCommands::execute should not be called directly".into())
    }

    fn group_name(&self) -> &str {
        "meta"
    }

    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl MetaCommands {
    pub fn handle_meta_tool(&self, tool: &str) -> Result<Value, String> {
        match tool {
            "ping" => Ok(serde_json::json!({ "message": "pong" })),
            "get_engine_version" => {
                if self.engine_version.is_empty() {
                    Err("Engine version not available".into())
                } else {
                    Ok(serde_json::json!({ "engine_version": self.engine_version }))
                }
            }
            "get_plugin_version" => {
                Ok(serde_json::json!({ "plugin_version": self.plugin_version }))
            }
            _ => Err(format!("Unknown meta tool: {}", tool)),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn meta_commands_can_handle() {
        let meta = MetaCommands::new();
        assert!(meta.can_handle("ping"));
        assert!(meta.can_handle("get_engine_version"));
        assert!(meta.can_handle("get_plugin_version"));
        assert!(!meta.can_handle("create_node"));
    }

    #[test]
    fn meta_commands_group_name() {
        let meta = MetaCommands::new();
        assert_eq!(meta.group_name(), "meta");
    }

    #[test]
    fn meta_commands_tool_names() {
        let meta = MetaCommands::new();
        assert_eq!(
            meta.tool_names(),
            &["ping", "get_engine_version", "get_plugin_version"]
        );
    }

    #[test]
    fn handle_ping() {
        let meta = MetaCommands::new();
        let result = meta.handle_meta_tool("ping").unwrap();
        assert_eq!(result["message"], "pong");
    }

    #[test]
    fn handle_get_engine_version() {
        let meta = MetaCommands::new().with_engine_version("4.6.0".into());
        let result = meta.handle_meta_tool("get_engine_version").unwrap();
        assert_eq!(result["engine_version"], "4.6.0");
    }

    #[test]
    fn handle_get_plugin_version() {
        let meta = MetaCommands::new();
        let result = meta.handle_meta_tool("get_plugin_version").unwrap();
        assert_eq!(result["plugin_version"], env!("CARGO_PKG_VERSION"));
    }

    #[test]
    fn handle_unknown_tool() {
        let meta = MetaCommands::new();
        let result = meta.handle_meta_tool("nonexistent");
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("Unknown meta tool"));
    }

    #[test]
    fn handle_engine_version_empty() {
        let meta = MetaCommands::new();
        let result = meta.handle_meta_tool("get_engine_version");
        assert!(result.is_err());
    }
}
