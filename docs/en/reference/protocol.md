# MCP Streamable HTTP Protocol

> Communication protocol between AI clients and the GDExtension via direct HTTP.

## Transport

- **Protocol**: HTTP POST/GET/DELETE (`http://127.0.0.1:9600/mcp`)
- **Port**: 9600 (overridable via `GODOT_MCP_HTTP_PORT` environment variable)
- **Encoding**: JSON-RPC 2.0 over HTTP, SSE for server-initiated events
- **Protocol Versions**: MCP `"2025-11-25"` / `"2025-03-26"`

## Session Establishment

```
1. Client → POST /mcp {"method": "initialize", ...}    → 200 + MCP-Session-Id
2. Client → GET /mcp (Accept: text/event-stream)       → 200 SSE stream
3. Client → POST /mcp {"method": "notifications/initialized"}  → 202
```

## Tool Invocation

```json
// Request: POST /mcp
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
        "name": "get_node_position",
        "arguments": {"node_path": "Player"}
    }
}

// Response: 200
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "content": [{"type": "text", "text": "{\"x\": 100.0, \"y\": 200.0}"}]
    }
}
```

## Supported MCP Methods

| Method | Description |
|--------|-------------|
| `initialize` | Create session, negotiate protocol version |
| `notifications/initialized` | Confirm initialization complete |
| `ping` | Keep-alive |
| `tools/list` | List available tools |
| `tools/call` | Execute a tool |
| `resources/list` | Empty array (not supported) |
| `prompts/list` | Empty array (not supported) |
| `logging/setLevel` | Set log level |
| `notifications/cancelled` | Cancel an in-flight request |

## SSE Events

```
id: 1
event: message
data: {"jsonrpc":"2.0","method":"notifications/message","params":{...}}
```

## Error Handling

| Scenario | Behavior |
|----------|----------|
| Invalid JSON | Returns `INVALID_REQUEST` |
| Invalid tool_call params | Returns `INVALID_REQUEST` |
| Unknown tool | Returns `UNKNOWN_TOOL` |
| Invalid HTTP Origin | Rejects non-`127.0.0.1`/`localhost`/`null` origins |
| Invalid session | Returns 400/401 |
