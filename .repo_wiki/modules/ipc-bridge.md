# Module: `ipc-bridge`

> WebSocket transport between `godot-mcp-server` and `godot_mcp_gdext`.
> Files: `crates/gdext/src/ipc/ws_server.rs` (server side, inside the editor) and `crates/server/src/bridge.rs` (client side, in the MCP server).

## Endpoint

- Hard-coded `ws://127.0.0.1:9500`. Port is configurable on the server with `--godot-port` but the GDExtension constant is `IpcWebSocketServer::new(9500, …)` in `editor_plugin.rs`. **Both sides must agree.**
- TCP only (`tokio::net::TcpListener::bind("127.0.0.1:{port}")`). No TLS, no Unix sockets.
- Single port. No fallback, no port discovery.

## GDExtension side: `IpcWebSocketServer`

`crates/gdext/src/ipc/ws_server.rs`. Constructed once per plugin enter, runs in a `tokio::spawn`ed task.

```rust
pub struct IpcWebSocketServer {
    port: u16,
    state: Arc<PluginState>,
    shutdown: Arc<Notify>,
    dispatcher: MainThreadDispatcher,
    registry: Vec<Box<dyn CommandHandler>>,
    broadcast_tx: broadcast::Sender<String>,
}
```

`broadcast_tx` is a `tokio::sync::broadcast::Sender<String>` with capacity 64. Used so dock UI (or any other in-editor source) can push notifications to every connected WS client simultaneously.

### `run()` loop

```
tokio::select!
    accept_result = listener.accept() => {
        spawn(handle_connection(ws, state, dispatcher, broadcast_rx, registry_tools))
    }
    _ = shutdown.notified() => { break }
```

`shutdown` is the `Arc<Notify>` set on plugin teardown. It cleanly breaks out of the accept loop without aborting in-flight connections (they end when the client closes the socket).

### Per-connection flow (`handle_connection`)

1. Send a `godot_ready` notification immediately with `engine_version`, `plugin_version`, `protocol_version: "1.0"`.
2. Split the WS into read/write halves.
3. Loop with `tokio::select!`:
   - **Incoming**: parse as `IpcRequest`, call `handle_request`, write the `IpcResponse` back.
   - **Broadcast**: receive a `String` (already-serialised notification JSON) from `broadcast_rx`, write it as-is.

If parsing fails the message is silently dropped (no error response — there's no `id` to address). All received text is also `eprintln!`'d for debug visibility.

### `handle_request` and `route_tool_call`

`handle_request` discriminates by `request.method`:

| `method` | Behaviour |
|----------|-----------|
| `"tool_call"` | Deserialise params as `ToolCallParams { tool, args }`. Call `route_tool_call(tool, &args, …)`. |
| anything else | Treat `request.method` itself as the tool name, `request.params` as args. (Backwards-compat for the very first version of the wire format; the current server never uses it.) |

`route_tool_call` (`ws_server.rs:209`):

1. `log_info(tool, "called args=…")`.
2. Try `MetaCommands.can_handle(tool)` → `handle_meta_tool` (sync, no dispatcher).
3. Else try `SceneCommands.can_handle(tool)` → `handle_scene_tool(tool, args, dispatcher).await` (async, every cmd is dispatched).
4. Else if `tool` is in `registry_tools` (the list passed in) → return `Err("Tool '<tool>' handler not yet implemented")`. This is the marker that the server-side schema is registered but no gdext routing arm exists.
5. Else → `Err("Unknown tool: <tool>")`.
6. Log success or failure.

`registry_tools` is materialised once at connection start by flattening every handler's `tool_names()` slice.

## Server side: `GodotBridge`

`crates/server/src/bridge.rs`. The MCP server's WebSocket client.

```rust
pub struct GodotBridge {
    writer: Arc<Mutex<SplitSink<…>>>,
    pending: Arc<DashMap<String, oneshot::Sender<Value>>>,
}
```

### `connect_with_handler(port, Option<Arc<GodotMcpHandler>>)`

1. `tokio_tungstenite::connect_async("ws://127.0.0.1:{port}")`.
2. Split, store the sink behind a `Mutex` for `call(...)` writes.
3. `tokio::spawn` a reader task that loops on `read.next().await`:
   - Try parse as `IpcResponse`. If `pending` has the `id`, fire the oneshot with `data` (or `{"error": message}` for `IpcResult::Error`).
   - Otherwise parse as `IpcNotification` and dispatch.

The handler argument is `Option<Arc<GodotMcpHandler>>` so unit tests can construct a bridge without a real handler.

### `call(method, params) -> anyhow::Result<Value>`

1. `Uuid::new_v4()`.
2. Build `IpcRequest { id, method, params }`, register oneshot sender in `pending`.
3. Serialise to JSON, write through the locked sink.
4. `rx.await?` — blocks until the reader fires the oneshot.

Failure modes:
- Connection drop after send → reader task ends → oneshot sender drops → `await` returns `Err`. `GodotMcpHandler::forward_tool_call` catches that, drops the cached bridge, and the next call re-connects.
- Timeout? **None.** A hung tool blocks indefinitely. There is no heartbeat, no `ping` keepalive, no read deadline. If you need a timeout, wrap the call in `tokio::time::timeout` at the call site.

### Notification dispatch

`handle_notification` (`bridge.rs:75`):

| `event` | Handling |
|---------|----------|
| `godot_ready` | `eprintln!` only. Informational. |
| `tool_list_updated` | If a handler is registered, deserialise `data` as `ToolListUpdate` and call `handler.update_tools(&update)` — the registry flips tool-enabled bits accordingly. |
| anything else | Log "Unknown notification: …" and ignore. |

## Wire format

The protocol page has the schema: [specification/ipc-protocol.md](../specification/ipc-protocol.md). Quick reminder of `tool_call`:

```jsonc
// → request from server
{ "id": "...", "method": "tool_call", "params": { "tool": "create_node", "args": { ... } } }

// ← response (success)
{ "id": "...", "status": "ok",    "data": { "node_path": "/root/Main/Player" } }

// ← response (error)
{ "id": "...", "status": "error", "code": -1, "message": "Node not found: /root/Foo" }

// ← notification (no id)
{ "type": "notification", "event": "tool_list_updated", "data": { "tools": [...] } }
```

## Concurrency model

- Multiple in-flight requests from one client are correlated by `id`. Order of response is not guaranteed (a fast tool can return before a slow one started earlier).
- Multiple WS connections are accepted but the dispatcher and broadcast channel are shared. In practice only one MCP server connects.
- `broadcast::Sender` has 64-slot lag tolerance. A slow client may miss notifications if it falls behind by more than 64 frames; for `tool_list_updated` that's acceptable because each update sends the full list.

## Things this module deliberately does not do

- **No reconnect logic on the GDExtension side.** It's a server; the client retries.
- **No reconnect timer on the bridge side.** The next failed `call` triggers `bridge.take()`; the next call re-connects.
- **No heartbeat.** Silent half-open connections are detected on the next send.
- **No authentication.** Anyone with access to `127.0.0.1:9500` can drive the editor. Bind address is hardcoded loopback, so that's "fine" for a developer-machine tool.
