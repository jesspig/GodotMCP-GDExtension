use futures_util::{SinkExt, StreamExt};
use serde_json::json;
use std::sync::Arc;

use godot_mcp_core::protocol::{IpcRequest, IpcResponse, IpcResult};
use godot_mcp_server::handler::GodotMcpHandler;

type Responder = Arc<dyn Fn(&IpcRequest) -> IpcResult + Send + Sync>;

struct MockGodotServer {
    shutdown: Option<tokio::sync::oneshot::Sender<()>>,
    port: u16,
}

impl MockGodotServer {
    async fn spawn(responder: Responder) -> Self {
        let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
        let port = listener.local_addr().unwrap().port();
        let (shutdown_tx, mut shutdown_rx) = tokio::sync::oneshot::channel::<()>();

        tokio::spawn(async move {
            tokio::select! {
                biased;
                _ = &mut shutdown_rx => {}
                result = listener.accept() => {
                    if let Ok((stream, _)) = result {
                        if let Ok(ws) = tokio_tungstenite::accept_async(stream).await {
                            let (mut write, mut read) = ws.split();
                            if let Some(Ok(tokio_tungstenite::tungstenite::Message::Text(text))) = read.next().await {
                                if let Ok(request) = serde_json::from_str::<IpcRequest>(&text) {
                                    let result = responder(&request);
                                    let response = IpcResponse {
                                        id: request.id,
                                        result,
                                    };
                                    let _ = write.send(tokio_tungstenite::tungstenite::Message::Text(
                                        serde_json::to_string(&response).unwrap()
                                    )).await;
                                }
                            }
                        }
                    }
                }
            }
        });

        Self {
            shutdown: Some(shutdown_tx),
            port,
        }
    }

    fn port(&self) -> u16 {
        self.port
    }
}

impl Drop for MockGodotServer {
    fn drop(&mut self) {
        if let Some(tx) = self.shutdown.take() {
            let _ = tx.send(());
        }
    }
}

#[tokio::test]
async fn e2e_ping_roundtrip() {
    let mock = MockGodotServer::spawn(Arc::new(|_req| IpcResult::Success {
        data: json!({"status": "pong"}),
    }))
    .await;

    let handler = GodotMcpHandler::new(mock.port());
    let result = handler.handle_tool_call("ping", json!({})).await.unwrap();
    assert_eq!(result, "{\"status\":\"pong\"}");
}

#[tokio::test]
async fn e2e_tool_call_with_params() {
    let mock = MockGodotServer::spawn(Arc::new(|req| {
        let tool = req
            .params
            .get("tool")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string();
        let args_val = req.params.get("args").cloned().unwrap_or(json!({}));
        IpcResult::Success {
            data: json!({
                "tool": tool,
                "args": args_val,
            }),
        }
    }))
    .await;

    let handler = GodotMcpHandler::new(mock.port());
    let result = handler
        .handle_tool_call(
            "set_node_position",
            json!({
                "node_path": "/root/Test",
                "x": 100,
                "y": 200
            }),
        )
        .await
        .unwrap();
    let parsed: serde_json::Value = serde_json::from_str(&result).unwrap();
    assert_eq!(parsed["tool"], "set_node_position");
    assert_eq!(parsed["args"]["node_path"], "/root/Test");
    assert_eq!(parsed["args"]["x"], 100);
    assert_eq!(parsed["args"]["y"], 200);
}

#[tokio::test]
async fn e2e_error_from_godot() {
    let mock = MockGodotServer::spawn(Arc::new(|_req| IpcResult::Error {
        code: -1,
        message: "Something went wrong".into(),
    }))
    .await;

    let handler = GodotMcpHandler::new(mock.port());
    let result = handler.handle_tool_call("ping", json!({})).await.unwrap();
    assert_eq!(result, "{\"error\":\"Something went wrong\"}");
}

#[tokio::test]
async fn e2e_get_server_version_offline() {
    let handler = GodotMcpHandler::new(9999);
    let result = handler
        .handle_tool_call("get_server_version", json!({}))
        .await
        .unwrap();
    assert!(!result.is_empty());
    assert!(result.contains('.'));
}

#[tokio::test]
async fn e2e_mcp_protocol_list_tools() {
    let handler = GodotMcpHandler::new(9999);
    let (enabled, total) = handler.registry().tool_count();
    assert_eq!(enabled, 125);
    assert_eq!(total, 125);
}
