use dashmap::DashMap;
use serde_json::json;
use std::sync::Arc;

use godot_mcp_core::tool_manifest::{ToolInfo, ToolListUpdate};

#[derive(Clone)]
pub struct ToolRegistry {
    tools: Arc<DashMap<String, ToolEntry>>,
}

struct ToolEntry {
    info: ToolInfo,
    enabled: bool,
}

impl ToolRegistry {
    pub fn new() -> Self {
        let registry = Self {
            tools: Arc::new(DashMap::new()),
        };
        registry.register_defaults();
        registry
    }

    fn register_defaults(&self) {
        let defaults: Vec<(&str, &str, serde_json::Value)> = vec![
            ("ping", "检测与 Godot 编辑器的连接状态", json!({"type":"object","properties":{},"required":[]})),
            ("get_engine_version", "获取 Godot 引擎版本号", json!({"type":"object","properties":{},"required":[]})),
            ("get_plugin_version", "获取 Godot MCP 插件版本号", json!({"type":"object","properties":{},"required":[]})),
            ("get_server_version", "获取 godot-mcp-server 版本号", json!({"type":"object","properties":{},"required":[]})),
        ];

        for (name, desc, schema) in defaults {
            self.tools.insert(
                name.to_string(),
                ToolEntry {
                    info: ToolInfo {
                        name: name.to_string(),
                        description: desc.to_string(),
                        input_schema: schema,
                        enabled: true,
                    },
                    enabled: true,
                },
            );
        }
    }

    #[allow(dead_code)]
    pub fn register_tool(&self, name: &str, description: &str, schema: serde_json::Value) {
        self.tools.insert(
            name.to_string(),
            ToolEntry {
                info: ToolInfo {
                    name: name.to_string(),
                    description: description.to_string(),
                    input_schema: schema,
                    enabled: true,
                },
                enabled: true,
            },
        );
    }

    #[allow(dead_code)]
    pub fn set_tool_enabled(&self, tool_name: &str, enabled: bool) -> bool {
        if let Some(mut entry) = self.tools.get_mut(tool_name) {
            entry.enabled = enabled;
            entry.info.enabled = enabled;
            true
        } else {
            false
        }
    }

    pub fn is_tool_enabled(&self, tool_name: &str) -> bool {
        self.tools
            .get(tool_name)
            .map(|e| e.enabled)
            .unwrap_or(false)
    }

    pub fn has_tool(&self, tool_name: &str) -> bool {
        self.tools.contains_key(tool_name)
    }

    pub fn get_enabled_tools(&self) -> Vec<ToolInfo> {
        self.tools
            .iter()
            .filter(|e| e.enabled)
            .map(|e| e.info.clone())
            .collect()
    }

    #[allow(dead_code)]
    pub fn get_all_tools(&self) -> Vec<ToolInfo> {
        self.tools.iter().map(|e| e.info.clone()).collect()
    }

    #[allow(dead_code)]
    pub fn update_from_notification(&self, update: &ToolListUpdate) {
        for tool_state in &update.tools {
            self.set_tool_enabled(&tool_state.name, tool_state.enabled);
        }
    }

    #[allow(dead_code)]
    pub fn tool_count(&self) -> (usize, usize) {
        let total = self.tools.len();
        let enabled = self.tools.iter().filter(|e| e.enabled).count();
        (enabled, total)
    }
}

impl Default for ToolRegistry {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use godot_mcp_core::tool_manifest::ToolState;

    #[test]
    fn new_registry_has_defaults() {
        let registry = ToolRegistry::new();
        let (enabled, total) = registry.tool_count();
        assert_eq!(total, 4);
        assert_eq!(enabled, 4);
    }

    #[test]
    fn all_defaults_enabled() {
        let registry = ToolRegistry::new();
        assert!(registry.is_tool_enabled("ping"));
        assert!(registry.is_tool_enabled("get_engine_version"));
        assert!(registry.is_tool_enabled("get_plugin_version"));
        assert!(registry.is_tool_enabled("get_server_version"));
    }

    #[test]
    fn disable_tool() {
        let registry = ToolRegistry::new();
        assert!(registry.set_tool_enabled("ping", false));
        assert!(!registry.is_tool_enabled("ping"));
        let (enabled, _) = registry.tool_count();
        assert_eq!(enabled, 3);
    }

    #[test]
    fn enable_tool() {
        let registry = ToolRegistry::new();
        registry.set_tool_enabled("ping", false);
        assert!(registry.set_tool_enabled("ping", true));
        assert!(registry.is_tool_enabled("ping"));
    }

    #[test]
    fn set_nonexistent_tool() {
        let registry = ToolRegistry::new();
        assert!(!registry.set_tool_enabled("nonexistent", true));
    }

    #[test]
    fn is_tool_enabled_unknown() {
        let registry = ToolRegistry::new();
        assert!(!registry.is_tool_enabled("unknown_tool"));
    }

    #[test]
    fn register_custom_tool() {
        let registry = ToolRegistry::new();
        registry.register_tool("custom_tool", "A custom tool", json!({}));
        assert!(registry.is_tool_enabled("custom_tool"));
        let (_, total) = registry.tool_count();
        assert_eq!(total, 5);
    }

    #[test]
    fn get_enabled_tools_respects_state() {
        let registry = ToolRegistry::new();
        registry.set_tool_enabled("ping", false);
        registry.set_tool_enabled("get_engine_version", false);
        let enabled = registry.get_enabled_tools();
        assert_eq!(enabled.len(), 2);
        let names: Vec<&str> = enabled.iter().map(|t| t.name.as_str()).collect();
        assert!(names.contains(&"get_plugin_version"));
        assert!(names.contains(&"get_server_version"));
    }

    #[test]
    fn update_from_notification() {
        let registry = ToolRegistry::new();
        let update = ToolListUpdate {
            tools: vec![
                ToolState { name: "ping".into(), enabled: false },
                ToolState { name: "get_engine_version".into(), enabled: false },
            ],
        };
        registry.update_from_notification(&update);
        assert!(!registry.is_tool_enabled("ping"));
        assert!(!registry.is_tool_enabled("get_engine_version"));
        assert!(registry.is_tool_enabled("get_plugin_version"));
        let (enabled, _) = registry.tool_count();
        assert_eq!(enabled, 2);
    }

    #[test]
    fn update_notification_unknown_tool_ignored() {
        let registry = ToolRegistry::new();
        let update = ToolListUpdate {
            tools: vec![
                ToolState { name: "nonexistent".into(), enabled: false },
            ],
        };
        registry.update_from_notification(&update);
        let (_, total) = registry.tool_count();
        assert_eq!(total, 4);
    }
}
