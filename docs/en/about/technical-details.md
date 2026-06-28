# Technical Details

## Threading Model

GodotMCP is **pure main-thread**, single-process architecture. There are no worker threads, no thread pools, no mutexes, and no lock-free data structures.

### Polling Loop

McpEditorPlugin::_process(delta) runs every editor frame and drives two subsystems:

`cpp
void McpEditorPlugin::_process(double delta) {
    http_server_.poll();      // Accept connections, parse HTTP, dispatch to MCP handler
    runtime_bridge_.poll();   // Read TCP data from game process bridge
}
`

This means:
- **All tool execution** happens on the main thread during _process
- **No thread safety concerns** — Godot API is freely callable
- **No MainThreadDispatcher** pattern needed
- **Blocking tools** (e.g., wait_for_bridge with timeout) use a frame-counter approach:
  `cpp
  if (elapsed_frames_++ > timeout_frames) { return ToolResult::err("TIMEOUT", "..."); }
  `

## Protocol Details

### MCP Streamable HTTP 2026-07-28

The server implements the MCP Streamable HTTP transport (2026-07-28 revision).

| Aspect | Detail |
|--------|--------|
| Endpoint | POST http://127.0.0.1:9600/mcp |
| Port config | GODOT_MCP_HTTP_PORT env var or ProjectSetting godot_mcp/http_port (default: 9600) |
| Host config | GODOT_MCP_HTTP_HOST env var (default: 127.0.0.1) |
| Protocol | JSON-RPC 2.0 |
| Session | **None** — stateless, no session IDs |
| GET endpoints | SSE streaming for real-time responses |
| SSE | Inline in POST response body |

### HTTP Headers

Requests must include:
- Content-Type: application/json
- Accept: application/json, text/event-stream

The server validates:
- Mcp-Method header matches method in request body
- Mcp-Name header matches params.name for tool calls (if present)
- Origin header for CORS

### Error Response Format

`json
{
    "jsonrpc": "2.0",
    "id": 1,
    "error": {
        "code": -32603,
        "message": "Tool execution failed",
        "data": {
            "success": false,
            "error": {
                "code": "TOOL_NOT_FOUND",
                "message": "Tool 'nonexistent' not found"
            }
        }
    }
}
`

### SSE Event Format

For streaming responses, events are formatted as:

`
event: message
data: {"jsonrpc":"2.0","id":1,"result":{"content":[{"type":"text","text":"..."}]}}

event: error
data: {"jsonrpc":"2.0","id":1,"error":{"code":-32603,"message":"..."}}
`

## Bridge Protocol

The editor connects to the game process via TCP on port 9601.

| Aspect | Detail |
|--------|--------|
| Port config | GODOT_MCP_BRIDGE_PORT env var (default: 9601) |
| Direction | Editor (client) → Game (server) |
| Framing | Newline-delimited JSON (\n) |
| Buffer | Max 64KB per message |
| Auto-detect | EditorInterface::is_playing_scene() for game start/stop |

### Message Flow

`
Editor → Game: {"cmd":"get_scene_tree","args":{},"id":1}
Game → Editor: {"ok":true,"data":{...},"id":1}
`

## Configuration Reference

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| GODOT_MCP_HTTP_PORT | 9600 | HTTP server port |
| GODOT_MCP_HTTP_HOST | 127.0.0.1 | HTTP server bind address |
| GODOT_MCP_BRIDGE_PORT | 9601 | Runtime bridge TCP port |

### ProjectSettings

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| godot_mcp/http_port | int | 9600 | HTTP server port (overrides env) |

## Startup Sequence

1. Godot loads the GDExtension → calls gdext_mcp_init()
2. McpEditorPlugin is registered as an EditorPlugin
3. _enter_tree() fires:
   - Creates HandlerRegistry and registers all 164 built-in tools via X-macro
   - Creates McpToolRegistry singleton for SDK access
   - Reads port configuration
   - Starts HttpServer on the configured port
4. _process() begins polling HTTP and bridge connections every frame
5. On editor exit, _exit_tree() shuts down the HTTP server and cleans up
