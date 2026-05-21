use std::sync::Arc;
use tokio::net::TcpListener;
use tokio::sync::Notify;
use tokio_tungstenite::{accept_async, WebSocketStream, tungstenite::protocol::Message};
use futures_util::{SinkExt, StreamExt};

use godot_mcp_core::protocol::{IpcRequest, IpcResponse, IpcResult, IpcNotification};

use crate::ipc::plugin_state::PluginState;

pub struct IpcWebSocketServer {
    port: u16,
    state: Arc<PluginState>,
    shutdown: Arc<Notify>,
}

impl IpcWebSocketServer {
    pub fn new(port: u16, state: Arc<PluginState>, shutdown: Arc<Notify>) -> Self {
        Self {
            port,
            state,
            shutdown,
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
                            match accept_async(stream).await {
                                Ok(ws) => {
                                    tokio::spawn(Self::handle_connection(ws, state));
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

        while let Some(msg_result) = read.next().await {
            match msg_result {
                Ok(msg) => {
                    if let Ok(text) = msg.into_text() {
                        eprintln!("[Godot MCP] Received: {}", text);

                        if let Ok(request) = serde_json::from_str::<IpcRequest>(&text) {
                            let response = Self::handle_request(&request, &state);

                            if let Ok(json) = serde_json::to_string(&response) {
                                eprintln!("[Godot MCP] Sending: {}", json);
                                let _ = write.send(Message::Text(json)).await;
                            }
                        }
                    }
                }
                Err(e) => {
                    eprintln!("[Godot MCP] WebSocket error: {}", e);
                    break;
                }
            }
        }

        eprintln!("[Godot MCP] IPC connection closed");
    }

    fn handle_request(request: &IpcRequest, state: &PluginState) -> IpcResponse {
        match request.method.as_str() {
            "ping" => IpcResponse {
                id: request.id.clone(),
                result: IpcResult::Success {
                    data: serde_json::json!({ "message": "pong" }),
                },
            },
            "get_engine_version" => IpcResponse {
                id: request.id.clone(),
                result: IpcResult::Success {
                    data: serde_json::json!({
                        "engine_version": state.engine_version
                    }),
                },
            },
            "get_plugin_version" => IpcResponse {
                id: request.id.clone(),
                result: IpcResult::Success {
                    data: serde_json::json!({
                        "plugin_version": state.plugin_version
                    }),
                },
            },
            _ => IpcResponse {
                id: request.id.clone(),
                result: IpcResult::Error {
                    code: -2,
                    message: format!("Unknown method: {}", request.method),
                },
            },
        }
    }
}