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
