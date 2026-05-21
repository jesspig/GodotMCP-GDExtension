use rmcp::ErrorData;
use rmcp::handler::server::ServerHandler;
use rmcp::model::*;
use rmcp::service::{RequestContext, RoleServer};
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

    pub async fn handle_tool_call(&self, tool_name: &str) -> Result<String, String> {
        match tool_name {
            "ping" => Ok(self.bridge_text("ping", "Godot 编辑器未连接").await),
            "get_engine_version" => Ok(self
                .bridge_text("get_engine_version", "Godot 编辑器未连接，无法获取引擎版本")
                .await),
            "get_plugin_version" => Ok(self
                .bridge_text("get_plugin_version", "Godot 编辑器未连接，无法获取插件版本")
                .await),
            "get_server_version" => Ok(SERVER_VERSION.to_string()),
            _ => Err(format!("Unknown tool: {}", tool_name)),
        }
    }

    #[cfg(test)]
    pub fn tool_names() -> &'static [&'static str] {
        &[
            "ping",
            "get_engine_version",
            "get_plugin_version",
            "get_server_version",
        ]
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
        match self.handle_tool_call(request.name.as_ref()).await {
            Ok(text) => Ok(CallToolResult::success(vec![Content::text(text)])),
            Err(msg) => Err(ErrorData::invalid_params(msg, None)),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_info() {
        let handler = GodotMcpHandler::new(9500);
        let info = handler.get_info();

        assert_eq!(info.protocol_version, ProtocolVersion::V_2025_03_26);
        assert_eq!(info.server_info.name, "Godot MCP");
        assert_eq!(info.server_info.version, SERVER_VERSION);
        assert!(info.instructions.is_some());
    }

    #[test]
    fn test_tool_names_contains_all_tools() {
        let names = GodotMcpHandler::tool_names();
        assert_eq!(names.len(), 4);
        assert!(names.contains(&"ping"));
        assert!(names.contains(&"get_engine_version"));
        assert!(names.contains(&"get_plugin_version"));
        assert!(names.contains(&"get_server_version"));
    }

    #[tokio::test]
    async fn test_get_server_version_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("get_server_version").await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), SERVER_VERSION);
    }

    #[tokio::test]
    async fn test_ping_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("ping").await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Godot 编辑器未连接");
    }

    #[tokio::test]
    async fn test_get_engine_version_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("get_engine_version").await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Godot 编辑器未连接，无法获取引擎版本");
    }

    #[tokio::test]
    async fn test_get_plugin_version_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("get_plugin_version").await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Godot 编辑器未连接，无法获取插件版本");
    }

    #[tokio::test]
    async fn test_unknown_tool() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("nonexistent").await;
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Unknown tool: nonexistent");
    }
}
