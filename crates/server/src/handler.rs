use rmcp::ErrorData;
use rmcp::handler::server::ServerHandler;
use rmcp::model::*;
use rmcp::service::{RequestContext, RoleServer};
use serde_json::json;
use std::sync::Arc;
use tokio::sync::Mutex;

use crate::bridge::GodotBridge;
use crate::tool_registry::ToolRegistry;

const SERVER_VERSION: &str = env!("CARGO_PKG_VERSION");

#[derive(Clone)]
pub struct GodotMcpHandler {
    godot_port: u16,
    bridge: Arc<Mutex<Option<Arc<GodotBridge>>>>,
    registry: ToolRegistry,
}

impl GodotMcpHandler {
    pub fn new(godot_port: u16) -> Self {
        Self {
            godot_port,
            bridge: Arc::new(Mutex::new(None)),
            registry: ToolRegistry::new(),
        }
    }

    #[allow(dead_code)]
    pub fn registry(&self) -> &ToolRegistry {
        &self.registry
    }

    async fn ensure_bridge(&self) -> Option<Arc<GodotBridge>> {
        let mut guard = self.bridge.lock().await;
        if guard.is_none() {
            match GodotBridge::connect_with_handler(self.godot_port, Some(Arc::new(self.clone())))
                .await
            {
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

    async fn forward_tool_call(
        &self,
        tool_name: &str,
        args: serde_json::Value,
        offline_message: &str,
    ) -> String {
        let bridge = match self.ensure_bridge().await {
            Some(b) => b,
            None => return offline_message.to_string(),
        };
        let params = json!({ "tool": tool_name, "args": args });
        match bridge.call("tool_call", params).await {
            Ok(result) => serde_json::to_string(&result).unwrap_or_else(|_| "{}".into()),
            Err(e) => {
                self.bridge.lock().await.take();
                eprintln!("[MCP Server] Godot connection lost: {}", e);
                format!("Godot 通信失败: {}", e)
            }
        }
    }

    pub async fn handle_tool_call(
        &self,
        tool_name: &str,
        args: serde_json::Value,
    ) -> Result<String, String> {
        if !self.registry.has_tool(tool_name) {
            return Err(format!("Unknown tool: {}", tool_name));
        }
        if !self.registry.is_tool_enabled(tool_name) {
            return Err(format!("Tool '{}' is disabled", tool_name));
        }

        if tool_name == "get_server_version" {
            return Ok(SERVER_VERSION.to_string());
        }

        let offline_msg = match tool_name {
            "ping" => "Godot 编辑器未连接",
            "get_engine_version" => "Godot 编辑器未连接，无法获取引擎版本",
            "get_plugin_version" => "Godot 编辑器未连接，无法获取插件版本",
            _ => "Godot 编辑器未连接",
        };

        Ok(self.forward_tool_call(tool_name, args, offline_msg).await)
    }

    #[allow(dead_code)]
    pub fn update_tools(&self, update: &godot_mcp_core::tool_manifest::ToolListUpdate) {
        self.registry.update_from_notification(update);
    }

    fn schema_from_value(
        value: &serde_json::Value,
    ) -> Arc<serde_json::Map<String, serde_json::Value>> {
        Arc::new(serde_json::from_value(value.clone()).unwrap_or_default())
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
        let tools: Vec<Tool> = self
            .registry
            .get_enabled_tools()
            .into_iter()
            .map(|info| {
                let schema = Self::schema_from_value(&info.input_schema);
                Tool::new(info.name, info.description, schema)
            })
            .collect();

        Ok(ListToolsResult {
            tools,
            next_cursor: None,
            meta: None,
        })
    }

    async fn call_tool(
        &self,
        request: CallToolRequestParams,
        _context: RequestContext<RoleServer>,
    ) -> Result<CallToolResult, ErrorData> {
        let args = request
            .arguments
            .clone()
            .map(serde_json::Value::Object)
            .unwrap_or(serde_json::Value::Object(serde_json::Map::new()));
        match self.handle_tool_call(request.name.as_ref(), args).await {
            Ok(text) => Ok(CallToolResult::success(vec![Content::text(text)])),
            Err(msg) => Err(ErrorData::invalid_params(msg, None)),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use godot_mcp_core::tool_manifest::{ToolListUpdate, ToolState};

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
    fn test_registry_defaults() {
        let handler = GodotMcpHandler::new(9500);
        let (enabled, total) = handler.registry().tool_count();
        assert_eq!(total, 49);
        assert_eq!(enabled, 49);
    }

    #[test]
    fn test_registry_disable() {
        let handler = GodotMcpHandler::new(9500);
        handler.registry().set_tool_enabled("ping", false);
        assert!(!handler.registry().is_tool_enabled("ping"));
    }

    #[tokio::test]
    async fn test_get_server_version_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler
            .handle_tool_call("get_server_version", json!({}))
            .await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), SERVER_VERSION);
    }

    #[tokio::test]
    async fn test_ping_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("ping", json!({})).await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Godot 编辑器未连接");
    }

    #[tokio::test]
    async fn test_get_engine_version_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler
            .handle_tool_call("get_engine_version", json!({}))
            .await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Godot 编辑器未连接，无法获取引擎版本");
    }

    #[tokio::test]
    async fn test_get_plugin_version_offline() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler
            .handle_tool_call("get_plugin_version", json!({}))
            .await;
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Godot 编辑器未连接，无法获取插件版本");
    }

    #[tokio::test]
    async fn test_unknown_tool() {
        let handler = GodotMcpHandler::new(9999);
        let result = handler.handle_tool_call("nonexistent", json!({})).await;
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Unknown tool: nonexistent");
    }

    #[tokio::test]
    async fn test_disabled_tool() {
        let handler = GodotMcpHandler::new(9999);
        handler.registry().set_tool_enabled("ping", false);
        let result = handler.handle_tool_call("ping", json!({})).await;
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("disabled"));
    }

    #[test]
    fn test_update_tools() {
        let handler = GodotMcpHandler::new(9500);
        let update = ToolListUpdate {
            tools: vec![ToolState {
                name: "ping".into(),
                enabled: false,
            }],
        };
        handler.update_tools(&update);
        assert!(!handler.registry().is_tool_enabled("ping"));
    }
}
