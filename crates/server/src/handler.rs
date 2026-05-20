use rmcp::handler::server::ServerHandler;
use rmcp::model::*;
use rmcp::tool;
use rmcp::ServiceExt;
use serde_json::json;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use dashmap::DashMap;
use tokio::sync::oneshot;

use crate::bridge::GodotBridge;
use godot_mcp_core::tool_manifest::{ToolListUpdate, ToolState};

#[derive(Clone)]
pub struct GodotMcpHandler {
    bridge: Arc<GodotBridge>,
    enabled_tools: Arc<DashMap<String, bool>>,
    tool_router_needs_rebuild: Arc<AtomicBool>,
}

impl GodotMcpHandler {
    pub async fn new(godot_port: u16) -> anyhow::Result<Self> {
        let bridge = Arc::new(GodotBridge::connect(godot_port).await?);
        
        Ok(Self {
            bridge,
            enabled_tools: Arc::new(DashMap::new()),
            tool_router_needs_rebuild: Arc::new(AtomicBool::new(false)),
        })
    }
    
    pub fn rebuild_tools(&self, update: &ToolListUpdate) {
        for tool in &update.tools {
            self.enabled_tools.insert(tool.name.clone(), tool.enabled);
        }
        self.tool_router_needs_rebuild.store(true, Ordering::SeqCst);
    }
    
    pub fn is_tool_enabled(&self, tool_name: &str) -> bool {
        self.enabled_tools.get(tool_name).map(|v| *v).unwrap_or(true)
    }
}

#[rmcp::async_trait]
impl ServerHandler for GodotMcpHandler {
    fn get_info(&self) -> ServerInfo {
        ServerInfo {
            protocol_version: ProtocolVersion::V_2025_03_26,
            capabilities: ServerCapabilities::builder()
                .enable_tools()
                .build(),
            server_info: Implementation::new("Godot MCP", env!("CARGO_PKG_VERSION")),
            instructions: Some("Godot MCP Server — 通过 AI 控制 Godot 编辑器".into()),
            ..Default::default()
        }
    }
    
    async fn list_tools(&self, _cursor: Option<String>) -> Result<ListToolsResult, McpError> {
        Ok(ListToolsResult {
            tools: vec![
                Tool::new(
                    "ping",
                    "Ping the Godot editor to check connection",
                    json!({
                        "type": "object",
                        "properties": {},
                        "required": []
                    })
                )
            ],
            next_cursor: None,
            meta: None,
        })
    }
    
    async fn call_tool(&self, name: &str, arguments: serde_json::Value) -> Result<CallToolResult, McpError> {
        match name {
            "ping" => {
                let result = self.bridge.call("ping", json!({})).await?;
                Ok(CallToolResult::success(vec![
                    Content::text(serde_json::to_string_pretty(&result).unwrap())
                ]))
            }
            _ => {
                Err(McpError::invalid_params(format!("Unknown tool: {}", name)))
            }
        }
    }
}
