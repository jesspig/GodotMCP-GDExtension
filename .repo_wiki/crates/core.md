# `godot-mcp-core`

> Pure-Rust types shared between the server and the GDExtension. Zero Godot or rmcp dependencies — only `serde`, `serde_json`, `uuid`.

## Files

| File | Contents |
|------|----------|
| `crates/core/src/lib.rs` | `pub mod protocol;` `pub mod tool_manifest;` (re-exports nothing — callers always go through the module name) |
| `crates/core/src/protocol.rs` | IPC wire types: `IpcRequest`, `IpcResponse`, `IpcResult`, `IpcNotification`, `ToolCallParams` |
| `crates/core/src/tool_manifest.rs` | Tool inventory types: `ToolManifest`, `ToolCategory`, `ToolInfo`, `ToolListUpdate`, `ToolState` |

## protocol.rs

The full spec lives in [`specification/ipc-protocol.md`](../specification/ipc-protocol.md). Quick reference:

```rust
pub struct IpcRequest  { pub id: String, pub method: String, pub params: Value }
pub struct IpcResponse { pub id: String, #[serde(flatten)] pub result: IpcResult }
pub enum   IpcResult   { Success { data: Value }, Error { code: i32, message: String } }   // #[serde(tag = "status")]
pub struct IpcNotification { #[serde(rename = "type")] pub msg_type: String, pub event: String, pub data: Value }
pub struct ToolCallParams { pub tool: String, #[serde(default)] pub args: Value }
```

`IpcResult` uses an internally-tagged enum (`status = "ok"|"error"`) flattened into `IpcResponse`. Wire JSON looks like `{"id":"...","status":"ok","data":{...}}` — there is no `result` envelope.

`ToolCallParams` is the payload carried inside `IpcRequest.params` when `method = "tool_call"`. Every MCP tool call from the server uses this single method; the actual tool name lives in `params.tool`. See [modules/command-routing.md](../modules/command-routing.md).

## tool_manifest.rs

`ToolInfo.enabled` defaults to `true` via `#[serde(default = "default_enabled")]`. `ToolListUpdate { tools: Vec<ToolState> }` is the payload of the `tool_list_updated` notification the GDExtension sends when the dock-UI toggle changes a tool's enabled state. The server's `ToolRegistry::update_from_notification` consumes it.

## Tests

15 round-trip / default-value tests in the module-level `#[cfg(test)] mod tests` blocks. No external state, no Godot, no tokio.

## When to touch this crate

- Adding a new IPC notification event → extend `IpcNotification` consumers in both processes (the type itself is opaque `Value`).
- Changing a wire format → both `server` and `gdext` will fail to compile. Bump versions together; there is no protocol negotiation.
- **Do not** add `godot` or `rmcp` here. The whole point of `core` is that it builds without either.
