use std::time::Duration;

use serde_json::{Value, json};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;
use tokio::time::timeout;

use super::protocol::{
    Diagnostic, JsonRpcNotification, JsonRpcRequest, JsonRpcResponse, LspNotification,
    PublishDiagnosticsParams,
};

const INIT_TIMEOUT: Duration = Duration::from_secs(2);
const DIAG_TIMEOUT: Duration = Duration::from_secs(3);
const READ_BUF_SIZE: usize = 65536;

/// Validate a GDScript file via Godot's built-in LSP server.
///
/// Performs a short-lived LSP session: connect -> initialize -> initialized
/// -> didOpen -> wait for publishDiagnostics -> shutdown -> exit.
///
/// Must be called from within a tokio runtime context.
pub async fn validate_via_lsp(
    port: u16,
    file_path: &str,
    file_uri: &str,
    source_code: &str,
    project_root_uri: &str,
) -> Result<Vec<Diagnostic>, String> {
    let addr = format!("127.0.0.1:{}", port);
    let mut stream = timeout(INIT_TIMEOUT, TcpStream::connect(&addr))
        .await
        .map_err(|_| format!("LSP connect timeout to {}", addr))?
        .map_err(|e| format!("LSP connect failed: {}", e))?;

    let mut next_id: u64 = 1;

    let init_params = json!({
        "processId": std::process::id(),
        "rootUri": project_root_uri,
        "capabilities": {
            "textDocument": {
                "publishDiagnostics": {"versionSupport": false},
                "synchronization": {"didSave": true}
            }
        },
        "workspaceFolders": [{
            "name": "project",
            "uri": project_root_uri
        }]
    });

    send_request(&mut stream, next_id, "initialize", init_params).await?;
    next_id += 1;

    let mut pending_diagnostics: Vec<Diagnostic> = Vec::new();
    let mut got_init_result = false;
    let init_deadline = tokio::time::Instant::now() + INIT_TIMEOUT;
    while tokio::time::Instant::now() < init_deadline && !got_init_result {
        match timeout(INIT_TIMEOUT, read_message(&mut stream)).await {
            Ok(Ok(msg)) => match classify(&msg) {
                Classified::Response(resp) => {
                    if let Some(error) = resp.error {
                        return Err(format!("LSP initialize error: {}", error.message));
                    }
                    got_init_result = true;
                }
                Classified::Notification(notif) => {
                    if notif.method == "textDocument/publishDiagnostics"
                        && let Ok(p) =
                            serde_json::from_value::<PublishDiagnosticsParams>(notif.params)
                    {
                        pending_diagnostics = p.diagnostics.unwrap_or_default();
                    }
                }
                Classified::Unknown => {}
            },
            Ok(Err(e)) => return Err(format!("LSP read error during init: {}", e)),
            Err(_) => return Err("LSP initialize timeout".into()),
        }
    }
    if !got_init_result {
        return Err("LSP initialize: no InitializeResult received".into());
    }

    send_notification(&mut stream, "initialized", json!({})).await?;

    let did_open_params = json!({
        "textDocument": {
            "uri": file_uri,
            "languageId": "gdscript",
            "version": 1,
            "text": source_code
        }
    });
    send_notification(&mut stream, "textDocument/didOpen", did_open_params).await?;

    let mut diagnostics = pending_diagnostics;
    let diag_deadline = tokio::time::Instant::now() + DIAG_TIMEOUT;
    while tokio::time::Instant::now() < diag_deadline {
        let remaining = diag_deadline.saturating_duration_since(tokio::time::Instant::now());
        if remaining.is_zero() {
            break;
        }
        match timeout(remaining, read_message(&mut stream)).await {
            Ok(Ok(msg)) => {
                if let Classified::Notification(notif) = classify(&msg)
                    && notif.method == "textDocument/publishDiagnostics"
                    && let Ok(p) = serde_json::from_value::<PublishDiagnosticsParams>(notif.params)
                {
                    if p.uri == file_uri || p.uri.ends_with(file_path) {
                        diagnostics = p.diagnostics.unwrap_or_default();
                        break;
                    } else {
                        diagnostics = p.diagnostics.unwrap_or_default();
                    }
                }
            }
            Ok(Err(e)) => return Err(format!("LSP read error: {}", e)),
            Err(_) => break,
        }
    }

    let _ = send_notification(
        &mut stream,
        "textDocument/didClose",
        json!({"textDocument": {"uri": file_uri}}),
    )
    .await;
    let _ = send_request(&mut stream, next_id, "shutdown", Value::Null).await;
    let _ = send_notification(&mut stream, "exit", Value::Null).await;

    Ok(diagnostics)
}

enum Classified {
    Response(JsonRpcResponse),
    Notification(LspNotification),
    Unknown,
}

fn classify(msg: &str) -> Classified {
    let v: Value = match serde_json::from_str(msg) {
        Ok(v) => v,
        Err(_) => return Classified::Unknown,
    };
    if v.get("method").is_some() {
        if v.get("id").is_some() {
            return Classified::Unknown;
        }
        if let Ok(n) = serde_json::from_value::<LspNotification>(v) {
            return Classified::Notification(n);
        }
        return Classified::Unknown;
    }
    if let Ok(r) = serde_json::from_value::<JsonRpcResponse>(v) {
        return Classified::Response(r);
    }
    Classified::Unknown
}

async fn send_request(
    stream: &mut TcpStream,
    id: u64,
    method: &str,
    params: Value,
) -> Result<(), String> {
    let req = JsonRpcRequest {
        jsonrpc: "2.0",
        id,
        method,
        params,
    };
    let body = serde_json::to_string(&req).map_err(|e| e.to_string())?;
    write_message(stream, &body).await
}

async fn send_notification(
    stream: &mut TcpStream,
    method: &str,
    params: Value,
) -> Result<(), String> {
    let notif = JsonRpcNotification {
        jsonrpc: "2.0",
        method,
        params,
    };
    let body = serde_json::to_string(&notif).map_err(|e| e.to_string())?;
    write_message(stream, &body).await
}

async fn write_message(stream: &mut TcpStream, body: &str) -> Result<(), String> {
    let frame = format!("Content-Length: {}\r\n\r\n{}", body.len(), body);
    stream
        .write_all(frame.as_bytes())
        .await
        .map_err(|e| format!("LSP write failed: {}", e))?;
    stream
        .flush()
        .await
        .map_err(|e| format!("LSP flush failed: {}", e))?;
    Ok(())
}

async fn read_message(stream: &mut TcpStream) -> Result<String, String> {
    let mut header = Vec::with_capacity(256);
    let mut byte = [0u8; 1];
    loop {
        let n = stream
            .read(&mut byte)
            .await
            .map_err(|e| format!("LSP read header: {}", e))?;
        if n == 0 {
            return Err("LSP stream closed during header read".into());
        }
        header.push(byte[0]);
        if header.ends_with(b"\r\n\r\n") {
            break;
        }
        if header.len() > 4096 {
            return Err("LSP header too large".into());
        }
    }
    let header_str = String::from_utf8_lossy(&header);
    let content_length: usize = header_str
        .lines()
        .find(|l| l.to_ascii_lowercase().starts_with("content-length:"))
        .and_then(|l| l.split(':').nth(1))
        .and_then(|v| v.trim().parse().ok())
        .ok_or_else(|| "LSP missing Content-Length".to_string())?;

    if content_length > READ_BUF_SIZE * 16 {
        return Err(format!("LSP body too large: {}", content_length));
    }

    let mut body = vec![0u8; content_length];
    stream
        .read_exact(&mut body)
        .await
        .map_err(|e| format!("LSP read body: {}", e))?;
    String::from_utf8(body).map_err(|e| format!("LSP body not UTF-8: {}", e))
}
