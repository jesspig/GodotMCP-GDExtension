# IPC Protocol Specification

> WebSocket communication protocol between `godot-mcp-server` and `godot_mcp_gdext.dll`.

## Transport

- **Protocol**: WebSocket (`ws://127.0.0.1:9500`)
- **Port**: 9500 (configurable via `--port` CLI arg)
- **Encoding**: JSON, UTF-8, `\n`-delimited messages
- **Connection**: At most one client — additional connections are rejected

## Type Definitions

All types defined in `crates/core/src/protocol.rs`.

### IpcRequest (Server → GDExt)

```json
{
    "tool": "ping",
    "args": {"node_path": "Player"},
    "id": "req-001"
}
```

- `tool`: string — tool name to invoke
- `args`: optional object — tool arguments
- `id`: string — request identifier for response matching

### IpcResponse (GDExt → Server)

```json
{
    "id": "req-001",
    "result": {"x": 100, "y": 200}
}
```

- `id`: string — matching request identifier
- `result`: value — tool call result

Error response example:

```json
{
    "id": "req-001",
    "result": {"error": "Node not found: 'MissingNode'"}
}
```

### IpcNotification (GDExt → Server)

```json
{
    "method": "mcp_log_message",
    "params": {"level": "info", "tool": "ping", "message": "Ping received"}
}
```

- `method`: string — notification type identifier
- `params`: object — notification data

## Tool Discovery

On startup, the server publishes tool schemas to the AI client via the `list_request_tools` response — no IPC request to gdext is needed. The tool registry is **statically declared** on the server side.

## Error Handling

### Protocol Level

| Situation | Behavior |
|-----------|----------|
| Invalid JSON | Connection closed |
| WebSocket disconnect | Server triggers reconnect logic |
| Timeout | Per-tool-call timeout in `bridge.rs` |
| Unknown tool | gdext returns `{"error": "unknown tool"}` |

### Application Level

All `cmd_*` functions return `json!({"error": "message"})` on error. The `pipe()` helper converts to `Result::Err`:

```rust
pipe(d.submit(move || {
    if something_wrong {
        return json!({"error": "something went wrong"});
    }
    json!({"status": "ok"})
}).await)
```

## Serialization

```rust
// Send
let request = IpcRequest { tool, args, id };
let bytes = serde_json::to_vec(&request)?;
ws_sender.send(bytes).await?;

// Receive
let bytes = ws_receiver.recv().await?;
let response: IpcResponse = serde_json::from_slice(&bytes)?;
```

## Godot Value Conversion

When tool params or return values contain Godot-specific types (Vector2, Color, etc.), values are converted using `j2v`/`v2j` functions. See [Scene Commands](../modules/scene-commands.md) for details.
