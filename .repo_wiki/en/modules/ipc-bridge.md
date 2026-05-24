# IPC Bridge (WebSocket)

> The communication bridge connecting `godot-mcp-server` and `godot_mcp_gdext`.

```mermaid
sequenceDiagram
    participant Server as godot-mcp-server
    participant GDExt as godot_mcp_gdext
    
    Note over Server,GDExt: Startup
    
    GDExt->>GDExt: IpcWebSocketServer::new("0.0.0.0:9500")
    GDExt->>GDExt: Bind listen
    GDExt->>GDExt: Accept connection
    GDExt-->>Server: WebSocket established
    
    Note over Server,GDExt: Tool call
    
    Server->>Server: forward_tool_call("ping", {})
    Server->>GDExt: send(IpcRequest{tool:"ping", args:{}, id:"1"})
    GDExt->>GDExt: route_tool_call("ping", {})
    GDExt->>GDExt: dispatcher.submit(cmd_ping)
    GDExt-->>Server: IpcResponse{id:"1", result:{"status":"ok"}}
    
    Note over Server,GDExt: Notification
    
    GDExt->>GDExt: tool produces log
    GDExt-->>Server: IpcNotification{method:"mcp_log", params:{...}}
```

## Wire Format

### Request (Server → GDExt)

```json
{
    "tool": "get_node_position",
    "args": {"node_path": "Player"},
    "id": "req-001"
}
```

### Response (GDExt → Server)

```json
{
    "id": "req-001",
    "result": {"x": 100.0, "y": 200.0}
}
```

Error response:
```json
{
    "id": "req-001",
    "result": {"error": "Node 'MissingNode' not found"}
}
```

### Notification (GDExt → Server)

```json
{
    "method": "mcp_log_message",
    "params": {"level": "info", "tool": "ping", "message": "Ping received"}
}
```

## Protocol Details

- Uses `serde_json::to_vec` / `from_slice` for serialization
- Messages separated by `\n` (JSON Lines style)
- `IpcWebSocketServer` held via `PluginState` static throughout Godot lifecycle
- Accepts only one connection (rejects additional clients)
- Auto-exits event loop on connection drop
- `bridge.rs` auto-reconnects on disconnect
