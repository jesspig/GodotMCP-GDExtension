use dashmap::DashMap;
use futures_util::{SinkExt, StreamExt};
use serde_json::Value;
use std::sync::Arc;
use tokio::sync::Mutex;
use tokio::sync::oneshot;
use tokio_tungstenite::connect_async;
use uuid::Uuid;

use godot_mcp_core::protocol::{IpcNotification, IpcRequest, IpcResponse, IpcResult};

pub struct GodotBridge {
    writer: Arc<
        Mutex<
            futures_util::stream::SplitSink<
                tokio_tungstenite::WebSocketStream<
                    tokio_tungstenite::MaybeTlsStream<tokio::net::TcpStream>,
                >,
                tokio_tungstenite::tungstenite::Message,
            >,
        >,
    >,
    pending: Arc<DashMap<String, oneshot::Sender<Value>>>,
}

impl GodotBridge {
    #[allow(dead_code)]
    pub async fn connect(port: u16) -> anyhow::Result<Self> {
        Self::connect_with_handler(port, None).await
    }

    pub async fn connect_with_handler(
        port: u16,
        handler: Option<Arc<crate::handler::GodotMcpHandler>>,
    ) -> anyhow::Result<Self> {
        let url = format!("ws://127.0.0.1:{}", port);
        let (ws, _) = connect_async(&url).await?;
        let (write, read) = ws.split();

        let writer = Arc::new(Mutex::new(write));
        let pending: Arc<DashMap<String, oneshot::Sender<Value>>> = Arc::new(DashMap::new());

        let pending_clone = pending.clone();
        tokio::spawn(async move {
            let mut read = read;
            while let Some(msg) = read.next().await {
                if let Ok(tokio_tungstenite::tungstenite::Message::Text(text)) = msg {
                    // Try parsing as response first
                    if let Ok(response) = serde_json::from_str::<IpcResponse>(&text) {
                        if let Some((_, tx)) = pending_clone.remove(&response.id) {
                            match response.result {
                                IpcResult::Success { data } => {
                                    let _ = tx.send(data);
                                }
                                IpcResult::Error { message, .. } => {
                                    let _ = tx.send(serde_json::json!({ "error": message }));
                                }
                            }
                        }
                    }
                    // Try parsing as notification
                    else if let Ok(notification) = serde_json::from_str::<IpcNotification>(&text)
                    {
                        Self::handle_notification(&notification, &handler);
                    }
                }
            }
        });

        Ok(Self { writer, pending })
    }

    fn handle_notification(
        notification: &IpcNotification,
        handler: &Option<Arc<crate::handler::GodotMcpHandler>>,
    ) {
        match notification.event.as_str() {
            "godot_ready" => {
                eprintln!("[MCP Server] Received godot_ready: {:?}", notification.data);
            }
            "tool_list_updated" => {
                eprintln!("[MCP Server] Received tool_list_updated");
                if let Some(h) = handler
                    && let Ok(update) = serde_json::from_value::<
                        godot_mcp_core::tool_manifest::ToolListUpdate,
                    >(notification.data.clone())
                {
                    h.update_tools(&update);
                    eprintln!("[MCP Server] Tools updated: {} tools", update.tools.len());
                }
            }
            _ => {
                eprintln!("[MCP Server] Unknown notification: {}", notification.event);
            }
        }
    }

    pub async fn call(&self, method: &str, params: Value) -> anyhow::Result<Value> {
        let id = Uuid::new_v4().to_string();
        let request = IpcRequest {
            id: id.clone(),
            method: method.into(),
            params,
        };

        let (tx, rx) = oneshot::channel();
        self.pending.insert(id.clone(), tx);

        let msg = serde_json::to_string(&request)?;
        self.writer
            .lock()
            .await
            .send(tokio_tungstenite::tungstenite::Message::Text(msg))
            .await?;

        let result = rx.await?;
        Ok(result)
    }
}
