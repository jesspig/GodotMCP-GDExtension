use tokio_tungstenite::connect_async;
use futures_util::{SinkExt, StreamExt};
use dashmap::DashMap;
use tokio::sync::oneshot;
use std::sync::Arc;
use tokio::sync::Mutex;
use serde_json::Value;
use uuid::Uuid;

use godot_mcp_core::protocol::{IpcRequest, IpcResponse, IpcResult};

pub struct GodotBridge {
    writer: Arc<Mutex<futures_util::stream::SplitSink<tokio_tungstenite::WebSocketStream<tokio_tungstenite::MaybeTlsStream<tokio::net::TcpStream>>, tokio_tungstenite::tungstenite::Message>>>,
    pending: Arc<DashMap<String, oneshot::Sender<Value>>>,
}

impl GodotBridge {
    pub async fn connect(port: u16) -> anyhow::Result<Self> {
        let url = format!("ws://127.0.0.1:{}", port);
        let (ws, _) = connect_async(&url).await?;
        let (write, read) = ws.split();
        
        let writer = Arc::new(Mutex::new(write));
        let pending: Arc<DashMap<String, oneshot::Sender<Value>>> = Arc::new(DashMap::new());
        
        let pending_clone = pending.clone();
        tokio::spawn(async move {
            let mut read = read;
            while let Some(msg) = read.next().await {
                match msg {
                    Ok(tokio_tungstenite::tungstenite::Message::Text(text)) => {
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
                    }
                    _ => {}
                }
            }
        });
        
        Ok(Self { writer, pending })
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
        self.writer.lock().await.send(
            tokio_tungstenite::tungstenite::Message::Text(msg.into())
        ).await?;
        
        let result = rx.await?;
        Ok(result)
    }
}
