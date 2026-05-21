use rmcp::handler::server::ServerHandler;
use rmcp::model::*;
use rmcp::service::{RequestContext, RoleServer};
use rmcp::ErrorData;
use serde_json::json;
use std::sync::Arc;
use tokio::sync::Mutex;

use crate::bridge::GodotBridge;

const SERVER_VERSION: &str = env!("CARGO_PKG_VERSION");

#[derive(Clone)]
pub struct GodotMcpHandler {
    godot_port: u16,
    bridge: Arc<Mutex<Option<Arc<GodotBridge>>>>,
}

impl GodotMcpHandler {
    pub fn new(godot_port: u16) -> Self {
        Self {
            godot_port,
            bridge: Arc::new(Mutex::new(None)),
        }
    }

    fn empty_schema() -> Arc<serde_json::Map<String, serde_json::Value>> {
        let schema: serde_json::Map<String, serde_json::Value> = serde_json::from_value(json!({
            "type": "object",
            "properties": {},
            "required": []
        }))
        .unwrap();
        Arc::new(schema)
    }

    async fn ensure_bridge(&self) -> Option<Arc<GodotBridge>> {
        let mut guard = self.bridge.lock().await;
        if guard.is_none() {
            match GodotBridge::connect(self.godot_port).await {
                Ok(bridge) => {
                    eprintln!(
                        "[MCP Server] Connected to Godot on port {}",
                        self.godot_port
                    );
                    let bridge = Arc::new(bridge);
                    *guard = Some(bridge.clone());
                    Some(bridge)
                }
                Err(_) => None,
            }
        } else {
            guard.as_ref().cloned()
        }
    }

    async fn bridge_text(&self, method: &str, offline_message: &str) -> String {
        let bridge = match self.ensure_bridge().await {
            Some(b) => b,
            None => return offline_message.to_string(),
        };
        match bridge.call(method, json!({})).await {
            Ok(result) => serde_json::to_string(&result).unwrap_or_else(|_| "{}".into()),
            Err(e) => {
                self.bridge.lock().await.take();
                eprintln!("[MCP Server] Godot connection lost: {}", e);
                format!("Godot 通信失败: {}", e)
            }
        }
    }

}

impl ServerHandler for GodotMcpHandler {
    fn get_info(&self) -> ServerInfo {
        let instructions = "Start Godot editor with the Godot MCP plugin installed. Connection will be established automatically.";

        let mut info = ServerInfo::default();
        info.protocol_version = ProtocolVersion::V_2025_03_26;
        info.capabilities = ServerCapabilities::builder().enable_tools().build();
        info.server_info = Implementation::new("Godot MCP", SERVER_VERSION);
        info.instructions = Some(instructions.into());
        info
    }

    async fn list_tools(
        &self,
        _request: Option<PaginatedRequestParams>,
        _context: RequestContext<RoleServer>,
    ) -> Result<ListToolsResult, ErrorData> {
        Ok(ListToolsResult {
            tools: vec![
                Tool::new(
                    "ping",
                    "检测与 Godot 编辑器的连接状态",
                    Self::empty_schema(),
                ),
                Tool::new(
                    "get_engine_version",
                    "获取 Godot 引擎版本号",
                    Self::empty_schema(),
                ),
                Tool::new(
                    "get_plugin_version",
                    "获取 Godot MCP 插件版本号",
                    Self::empty_schema(),
                ),
                Tool::new(
                    "get_server_version",
                    "获取 godot-mcp-server 版本号",
                    Self::empty_schema(),
                ),
            ],
            next_cursor: None,
            meta: None,
        })
    }

    async fn call_tool(
        &self,
        request: CallToolRequestParams,
        _context: RequestContext<RoleServer>,
    ) -> Result<CallToolResult, ErrorData> {
        match request.name.as_ref() {
            "ping" => Ok(CallToolResult::success(vec![Content::text(
                self.bridge_text("ping", "Godot 编辑器未连接").await,
            )])),
            "get_engine_version" => Ok(CallToolResult::success(vec![Content::text(
                self.bridge_text("get_engine_version", "Godot 编辑器未连接，无法获取引擎版本")
                    .await,
            )])),
            "get_plugin_version" => Ok(CallToolResult::success(vec![Content::text(
                self.bridge_text("get_plugin_version", "Godot 编辑器未连接，无法获取插件版本")
                    .await,
            )])),
            "get_server_version" => Ok(CallToolResult::success(vec![Content::text(
                SERVER_VERSION.to_string(),
            )])),
            _ => Err(ErrorData::invalid_params(
                format!("Unknown tool: {}", request.name),
                None,
            )),
        }
    }
}