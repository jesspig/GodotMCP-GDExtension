use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolManifest {
    pub categories: Vec<ToolCategory>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolCategory {
    pub name: String,
    pub display_name: String,
    pub tools: Vec<ToolInfo>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolInfo {
    pub name: String,
    pub description: String,
    pub input_schema: Value,
    #[serde(default = "default_enabled")]
    pub enabled: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolListUpdate {
    pub tools: Vec<ToolState>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolState {
    pub name: String,
    pub enabled: bool,
}

fn default_enabled() -> bool {
    true
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn test_tool_manifest_roundtrip() {
        let manifest = ToolManifest {
            categories: vec![ToolCategory {
                name: "scene".into(),
                display_name: "Scene Management".into(),
                tools: vec![ToolInfo {
                    name: "get_scene_tree".into(),
                    description: "Get scene tree".into(),
                    input_schema: json!({"type": "object"}),
                    enabled: true,
                }],
            }],
        };
        let json_str = serde_json::to_string(&manifest).unwrap();
        let de: ToolManifest = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.categories.len(), 1);
        assert_eq!(de.categories[0].name, "scene");
        assert_eq!(de.categories[0].tools.len(), 1);
        assert_eq!(de.categories[0].tools[0].name, "get_scene_tree");
    }

    #[test]
    fn test_tool_info_default_enabled() {
        let json_str = r#"{"name":"ping","description":"test","input_schema":{}}"#;
        let info: ToolInfo = serde_json::from_str(json_str).unwrap();
        assert!(info.enabled);
    }

    #[test]
    fn test_tool_info_explicit_disabled() {
        let json_str =
            r#"{"name":"debug_tool","description":"test","input_schema":{},"enabled":false}"#;
        let info: ToolInfo = serde_json::from_str(json_str).unwrap();
        assert!(!info.enabled);
    }

    #[test]
    fn test_tool_list_update_roundtrip() {
        let update = ToolListUpdate {
            tools: vec![
                ToolState {
                    name: "ping".into(),
                    enabled: true,
                },
                ToolState {
                    name: "debug_draw".into(),
                    enabled: false,
                },
            ],
        };
        let json_str = serde_json::to_string(&update).unwrap();
        let de: ToolListUpdate = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.tools.len(), 2);
        assert!(de.tools[0].enabled);
        assert!(!de.tools[1].enabled);
    }

    #[test]
    fn test_tool_state_json_format() {
        let json_str =
            r#"{"tools":[{"name":"ping","enabled":true},{"name":"stop","enabled":false}]}"#;
        let update: ToolListUpdate = serde_json::from_str(json_str).unwrap();
        assert_eq!(update.tools[0].name, "ping");
        assert_eq!(update.tools[1].name, "stop");
    }
}
