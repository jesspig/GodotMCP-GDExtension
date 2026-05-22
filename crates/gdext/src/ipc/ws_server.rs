use futures_util::{SinkExt, StreamExt};
use std::sync::Arc;
use tokio::net::TcpListener;
use tokio::sync::{Notify, broadcast};
use tokio_tungstenite::{WebSocketStream, accept_async, tungstenite::protocol::Message};

use godot_mcp_core::protocol::{
    IpcNotification, IpcRequest, IpcResponse, IpcResult, ToolCallParams,
};

use crate::commands::CommandHandler;
use crate::commands::meta::MetaCommands;
use crate::commands::node::NodeCommands;
use crate::commands::scene::SceneCommands;
use crate::commands::script_cs::ScriptCsCommands;
use crate::commands::script_gd::ScriptGdCommands;
use crate::commands::search::SearchCommands;
use crate::dispatcher::MainThreadDispatcher;
use crate::ipc::plugin_state::PluginState;
use crate::logging::{log_error, log_info};

pub struct IpcWebSocketServer {
    port: u16,
    state: Arc<PluginState>,
    shutdown: Arc<Notify>,
    dispatcher: MainThreadDispatcher,
    registry: Vec<Box<dyn CommandHandler>>,
    broadcast_tx: broadcast::Sender<String>,
}

impl IpcWebSocketServer {
    pub fn new(
        port: u16,
        state: Arc<PluginState>,
        shutdown: Arc<Notify>,
        dispatcher: MainThreadDispatcher,
        registry: Vec<Box<dyn CommandHandler>>,
    ) -> Self {
        let (broadcast_tx, _) = broadcast::channel(64);
        Self {
            port,
            state,
            shutdown,
            dispatcher,
            registry,
            broadcast_tx,
        }
    }

    pub fn broadcast_tx(&self) -> broadcast::Sender<String> {
        self.broadcast_tx.clone()
    }

    #[allow(dead_code)]
    pub fn broadcast_notification(&self, notification: &IpcNotification) {
        if let Ok(json) = serde_json::to_string(notification) {
            let _ = self.broadcast_tx.send(json);
        }
    }

    pub async fn run(&mut self) -> anyhow::Result<()> {
        let addr = format!("127.0.0.1:{}", self.port);

        eprintln!("[Godot MCP] IPC WebSocket server starting on {}...", addr);

        let listener = TcpListener::bind(&addr).await?;

        eprintln!("[Godot MCP] IPC WebSocket server listening on {}", addr);

        loop {
            tokio::select! {
                accept_result = listener.accept() => {
                    match accept_result {
                        Ok((stream, addr)) => {
                            eprintln!("[Godot MCP] New IPC connection from {}", addr);

                            let state = self.state.clone();
                            let dispatcher = self.dispatcher.clone();
                            let broadcast_rx = self.broadcast_tx.subscribe();
                            let registry: Vec<String> = self.registry.iter()
                                .flat_map(|h| h.tool_names().iter().map(|s| s.to_string()))
                                .collect();
                            match accept_async(stream).await {
                                Ok(ws) => {
                                    tokio::spawn(Self::handle_connection(
                                        ws, state, dispatcher, broadcast_rx, registry,
                                    ));
                                }
                                Err(e) => {
                                    eprintln!("[Godot MCP] WebSocket accept error: {}", e);
                                }
                            }
                        }
                        Err(e) => {
                            eprintln!("[Godot MCP] Listener accept error: {}", e);
                            break;
                        }
                    }
                }
                _ = self.shutdown.notified() => {
                    eprintln!("[Godot MCP] WebSocket server received shutdown signal");
                    break;
                }
            }
        }

        eprintln!("[Godot MCP] WebSocket server stopped");
        Ok(())
    }

    async fn handle_connection(
        mut ws: WebSocketStream<tokio::net::TcpStream>,
        state: Arc<PluginState>,
        dispatcher: MainThreadDispatcher,
        mut broadcast_rx: broadcast::Receiver<String>,
        registry_tools: Vec<String>,
    ) {
        eprintln!("[Godot MCP] IPC connection established");

        let ready_notif = IpcNotification {
            msg_type: "notification".into(),
            event: "godot_ready".into(),
            data: serde_json::json!({
                "engine_version": state.engine_version,
                "plugin_version": state.plugin_version,
                "protocol_version": "1.0"
            }),
        };

        if let Ok(json) = serde_json::to_string(&ready_notif) {
            let _ = ws.send(Message::Text(json)).await;
        }

        let (mut write, mut read) = ws.split();

        loop {
            tokio::select! {
                msg_result = read.next() => {
                    match msg_result {
                        Some(Ok(msg)) => {
                            if let Ok(text) = msg.into_text() {
                                eprintln!("[Godot MCP] Received: {}", text);

                                if let Ok(request) = serde_json::from_str::<IpcRequest>(&text) {
                                    let response = Self::handle_request(
                                        &request, &state, &dispatcher, &registry_tools,
                                    ).await;

                                    if let Ok(json) = serde_json::to_string(&response) {
                                        eprintln!("[Godot MCP] Sending: {}", json);
                                        let _ = write.send(Message::Text(json)).await;
                                    }
                                }
                            }
                        }
                        Some(Err(e)) => {
                            eprintln!("[Godot MCP] WebSocket error: {}", e);
                            break;
                        }
                        None => break,
                    }
                }
                broadcast_msg = broadcast_rx.recv() => {
                    if let Ok(json) = broadcast_msg {
                        eprintln!("[Godot MCP] Broadcasting: {}", json);
                        let _ = write.send(Message::Text(json)).await;
                    }
                }
            }
        }

        eprintln!("[Godot MCP] IPC connection closed");
    }

    async fn handle_request(
        request: &IpcRequest,
        state: &PluginState,
        dispatcher: &MainThreadDispatcher,
        registry_tools: &[String],
    ) -> IpcResponse {
        let result = if request.method == "tool_call" {
            let params: ToolCallParams = match serde_json::from_value(request.params.clone()) {
                Ok(p) => p,
                Err(e) => {
                    return IpcResponse {
                        id: request.id.clone(),
                        result: IpcResult::Error {
                            code: -3,
                            message: format!("Invalid tool_call params: {}", e),
                        },
                    };
                }
            };
            Self::route_tool_call(
                &params.tool,
                &params.args,
                state,
                dispatcher,
                registry_tools,
            )
            .await
        } else {
            Self::route_tool_call(
                &request.method,
                &request.params,
                state,
                dispatcher,
                registry_tools,
            )
            .await
        };

        match result {
            Ok(data) => IpcResponse {
                id: request.id.clone(),
                result: IpcResult::Success { data },
            },
            Err(msg) => IpcResponse {
                id: request.id.clone(),
                result: IpcResult::Error {
                    code: -1,
                    message: msg,
                },
            },
        }
    }

    async fn route_tool_call(
        tool: &str,
        args: &serde_json::Value,
        state: &PluginState,
        dispatcher: &MainThreadDispatcher,
        registry_tools: &[String],
    ) -> Result<serde_json::Value, String> {
        log_info(tool, &format!("called args={}", args));

        // MetaCommands: simple state queries, no dispatcher needed
        let meta = MetaCommands::new().with_engine_version(state.engine_version.clone());
        let result = if meta.can_handle(tool) {
            meta.handle_meta_tool(tool)
        } else {
            // NodeCommands: node operations via dispatcher
            let node = NodeCommands::new();
            if node.can_handle(tool) {
                node.handle_node_tool(tool, args, dispatcher).await
            } else {
                // SceneCommands: scene file and editor tab operations via dispatcher
                let scene = SceneCommands::new();
                if scene.can_handle(tool) {
                    scene.handle_scene_tool(tool, args, dispatcher).await
                } else {
                    let script_gd = ScriptGdCommands::new();
                    if script_gd.can_handle(tool) {
                        script_gd
                            .handle_script_gd_tool(tool, args, dispatcher)
                            .await
                    } else {
                        let script_cs = ScriptCsCommands::new();
                        if script_cs.can_handle(tool) {
                            script_cs
                                .handle_script_cs_tool(tool, args, dispatcher)
                                .await
                        } else {
                            let search = SearchCommands::new();
                            if search.can_handle(tool) {
                                search.handle_search_tool(tool, args, dispatcher).await
                            } else if registry_tools.contains(&tool.to_string()) {
                                Err(format!("Tool '{}' handler not yet implemented", tool))
                            } else {
                                Err(format!("Unknown tool: {}", tool))
                            }
                        }
                    }
                }
            }
        };

        match &result {
            Ok(v) => log_info(tool, &format!("ok result={}", v)),
            Err(e) => log_error(tool, &format!("failed: {}", e)),
        }
        result
    }
}
