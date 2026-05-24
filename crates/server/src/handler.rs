use rmcp::ErrorData;
use rmcp::handler::server::ServerHandler;
use rmcp::model::*;
use rmcp::service::{RequestContext, RoleServer};
use serde_json::json;
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::Mutex;

use crate::bridge::GodotBridge;
use crate::tool_registry::ToolRegistry;

const SERVER_VERSION: &str = env!("CARGO_PKG_VERSION");
const RECONNECT_MAX_ATTEMPTS: u32 = 5;
const RECONNECT_INITIAL_DELAY: Duration = Duration::from_secs(1);
const RECONNECT_MAX_DELAY: Duration = Duration::from_secs(30);

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
        let guard = self.bridge.lock().await;
        if guard.is_some() {
            return guard.as_ref().cloned();
        }
        drop(guard);

        let mut delay = RECONNECT_INITIAL_DELAY;
        for attempt in 0..RECONNECT_MAX_ATTEMPTS {
            let mut guard = self.bridge.lock().await;
            if guard.is_some() {
                return guard.as_ref().cloned();
            }
            match GodotBridge::connect_with_handler(self.godot_port, Some(Arc::new(self.clone())))
                .await
            {
                Ok(bridge) => {
                    eprintln!(
                        "[MCP Server] Connected to Godot on port {} (attempt {})",
                        self.godot_port,
                        attempt + 1
                    );
                    let bridge = Arc::new(bridge);
                    *guard = Some(bridge.clone());
                    return Some(bridge);
                }
                Err(e) => {
                    if attempt + 1 < RECONNECT_MAX_ATTEMPTS {
                        eprintln!(
                            "[MCP Server] Connection attempt {} failed: {}. Retrying in {:?}...",
                            attempt + 1,
                            e,
                            delay
                        );
                        drop(guard);
                        tokio::time::sleep(delay).await;
                        delay = (delay * 2).min(RECONNECT_MAX_DELAY);
                    } else {
                        eprintln!(
                            "[MCP Server] All {} connection attempts failed. Last error: {}",
                            RECONNECT_MAX_ATTEMPTS, e
                        );
                        return None;
                    }
                }
            }
        }
        None
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

        match tool_name {
            "get_server_version" => return Ok(SERVER_VERSION.to_string()),
            "godot_editor_open" => return godot_editor_open(args),
            "godot_editor_close" => return godot_editor_close(),
            "godot_editor_restart" => return self.godot_editor_restart(args).await,
            _ => {}
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

    async fn godot_editor_restart(&self, args: serde_json::Value) -> Result<String, String> {
        let godot_path = get_godot_path()?;
        let exe_name = std::path::Path::new(&godot_path)
            .file_name()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_default();

        let was_running = kill_process_by_name(&exe_name);
        if was_running {
            self.bridge.lock().await.take();
            tokio::time::sleep(Duration::from_millis(500)).await;
        }

        let project_path = resolve_project_path(&args);
        if !std::path::Path::new(&project_path).exists() {
            return Err(format!("项目目录不存在: {}", project_path));
        }

        let child = std::process::Command::new(&godot_path)
            .args(["--editor", "--path", &project_path])
            .spawn()
            .map_err(|e| format!("重启 Godot 编辑器失败: {}", e))?;

        let pid = child.id();
        Ok(serde_json::json!({
            "status": "restarted",
            "pid": pid,
            "was_running": was_running,
            "godot_path": godot_path,
            "project_path": project_path,
            "hint": "Editor is restarting. First tool call will auto-reconnect with retry."
        })
        .to_string())
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

fn get_godot_path() -> Result<String, String> {
    std::env::var("GODOT_PATH").map_err(|_| {
        "GODOT_PATH 环境变量未设置。请在 MCP 客户端配置中添加:\n\
         \"env\": { \"GODOT_PATH\": \"<path/to/godot.exe>\" }"
            .to_string()
    })
}

fn resolve_project_path(args: &serde_json::Value) -> String {
    let p = args
        .get("project_path")
        .and_then(|v| v.as_str())
        .unwrap_or("godot");
    if std::path::Path::new(p).is_absolute() {
        p.to_string()
    } else {
        std::env::current_dir()
            .map(|d| d.join(p).to_string_lossy().to_string())
            .unwrap_or_else(|_| p.to_string())
    }
}

fn godot_editor_open(args: serde_json::Value) -> Result<String, String> {
    let godot_path = get_godot_path()?;
    let project_path = resolve_project_path(&args);

    if !std::path::Path::new(&project_path).exists() {
        return Err(format!("项目目录不存在: {}", project_path));
    }

    let child = std::process::Command::new(&godot_path)
        .args(["--editor", "--path", &project_path])
        .spawn()
        .map_err(|e| format!("启动 Godot 编辑器失败: {}", e))?;

    let pid = child.id();
    Ok(serde_json::json!({
        "status": "opened",
        "pid": pid,
        "godot_path": godot_path,
        "project_path": project_path
    })
    .to_string())
}

fn godot_editor_close() -> Result<String, String> {
    let godot_path = get_godot_path()?;
    let exe_name = std::path::Path::new(&godot_path)
        .file_name()
        .map(|n| n.to_string_lossy().to_string())
        .unwrap_or_default();

    let killed = kill_process_by_name(&exe_name);

    Ok(serde_json::json!({
        "status": if killed { "closed" } else { "not_running" },
        "process_name": exe_name
    })
    .to_string())
}

fn kill_process_by_name(name: &str) -> bool {
    if cfg!(target_os = "windows") {
        std::process::Command::new("taskkill")
            .args(["/F", "/IM", name])
            .output()
            .map(|o| o.status.success())
            .unwrap_or(false)
    } else {
        std::process::Command::new("pkill")
            .args(["-f", name])
            .output()
            .map(|o| o.status.success())
            .unwrap_or(false)
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
        assert_eq!(total, 125);
        assert_eq!(enabled, 125);
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

    #[tokio::test]
    async fn test_godot_editor_open_no_env() {
        let handler = GodotMcpHandler::new(9999);
        let saved = std::env::var("GODOT_PATH").ok();
        unsafe {
            std::env::remove_var("GODOT_PATH");
        }
        let result = handler
            .handle_tool_call("godot_editor_open", json!({}))
            .await;
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("GODOT_PATH"));
        if let Some(v) = saved {
            unsafe {
                std::env::set_var("GODOT_PATH", v);
            }
        }
    }

    #[tokio::test]
    async fn test_godot_editor_close_no_env() {
        let handler = GodotMcpHandler::new(9999);
        let saved = std::env::var("GODOT_PATH").ok();
        unsafe {
            std::env::remove_var("GODOT_PATH");
        }
        let result = handler
            .handle_tool_call("godot_editor_close", json!({}))
            .await;
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("GODOT_PATH"));
        if let Some(v) = saved {
            unsafe {
                std::env::set_var("GODOT_PATH", v);
            }
        }
    }

    #[tokio::test]
    async fn test_godot_editor_restart_no_env() {
        let handler = GodotMcpHandler::new(9999);
        let saved = std::env::var("GODOT_PATH").ok();
        unsafe {
            std::env::remove_var("GODOT_PATH");
        }
        let result = handler
            .handle_tool_call("godot_editor_restart", json!({}))
            .await;
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("GODOT_PATH"));
        if let Some(v) = saved {
            unsafe {
                std::env::set_var("GODOT_PATH", v);
            }
        }
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
