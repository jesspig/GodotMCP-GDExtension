# Module: `command-routing`

> Where MCP tool calls become Rust function calls. Spans `crates/server/src/handler.rs`, `crates/gdext/src/ipc/ws_server.rs`, `crates/gdext/src/commands/`.

## The two-sided contract

Every tool exists in **two** places. Both sides must agree on the name.

| Side | File | What it adds |
|------|------|--------------|
| MCP server | `crates/server/src/tool_registry.rs::register_defaults` | `(name, description, JSON schema)` tuple. Determines what `list_tools` returns. |
| GDExtension | `crates/gdext/src/commands/scene.rs::handle_scene_tool` (or `meta.rs`) | Routing arm + `cmd_*` implementation. Determines what actually runs. |

Symptoms of mismatch:

| Registered on server, missing on gdext | `Tool '<name>' handler not yet implemented` |
| Implemented on gdext, missing on server | MCP client never sees the tool in `list_tools`; if it tries anyway, server returns `Unknown tool: <name>` |

**Tests count tools.** `tool_registry.rs::tests::new_registry_has_defaults` and `handler.rs::tests::test_registry_defaults` assert exact tool totals (currently 35). Adjacent tests assert post-disable counts. Update them in the same change or `cargo test` breaks.

## Wire-level dispatch

The MCP server only ever calls **one** IPC method: `"tool_call"`. The actual tool name is inside `params.tool`. This was a deliberate redesign — the previous protocol used `method` as the tool name directly, which forced the server to hard-code special cases for every tool. Now the server has a single forwarder.

```
MCP call_tool { name: "create_node", arguments: { ... } }
        │
        ▼
GodotMcpHandler::call_tool
        │  build args = Value::Object(arguments)
        ▼
GodotMcpHandler::handle_tool_call("create_node", args)
        ├─ if !registry.has_tool → Err("Unknown tool: …")
        ├─ if !registry.is_tool_enabled → Err("Tool '…' is disabled")
        ├─ if name == "get_server_version" → Ok(CARGO_PKG_VERSION)   // local short-circuit
        └─ else: forward_tool_call(name, args, offline_msg)
                    │
                    ▼  bridge.call("tool_call", { "tool": name, "args": args })
                    │
                    │  ── WebSocket ──►
                    ▼
IpcWebSocketServer::handle_request
        │  method == "tool_call" → ToolCallParams { tool, args }
        ▼
route_tool_call(tool, args, state, dispatcher, registry_tools)
        ├─ log_info(tool, "called args=…")
        ├─ MetaCommands.can_handle(tool) ? handle_meta_tool(tool)        // sync
        ├─ SceneCommands.can_handle(tool) ? handle_scene_tool(...).await // async via dispatcher
        ├─ tool in registry_tools ? Err("handler not yet implemented")
        ├─ else: Err("Unknown tool: …")
        └─ log_info/log_error with result/error
```

For a worked example see [overview/architecture.md](../overview/architecture.md#request-flow-worked-example).

## `CommandHandler` trait

`crates/gdext/src/commands/mod.rs`:

```rust
pub trait CommandHandler: Send + Sync {
    fn can_handle(&self, tool: &str) -> bool;
    fn execute(&self, args: &Value, dispatcher: &MainThreadDispatcher) -> Result<Value, String>;
    fn group_name(&self) -> &str;
    fn tool_names(&self) -> &[&str];
}

pub fn create_registry() -> Vec<Box<dyn CommandHandler>> {
    vec![Box::new(MetaCommands::new()), Box::new(SceneCommands::new())]
}
```

In practice **`execute` is never called**. Both implementers return `Err("… should not be called directly")` from it. Real routing goes through the specialised methods:

- `MetaCommands::handle_meta_tool(tool: &str) -> Result<Value, String>` — sync; pure state queries, no `EditorInterface`.
- `SceneCommands::handle_scene_tool(tool: &str, args: &Value, d: &MainThreadDispatcher) -> Result<Value, String>` — async; dispatches to a `cmd_*` free function via `d.submit(...).await`.

`create_registry()` is still useful — it's how `IpcWebSocketServer` produces the `registry_tools: Vec<String>` list for the "handler not yet implemented" fallback. The `tool_names()` method on each handler returns a `&[&str]`; `lib_server` flattens those at connection accept time.

## `MetaCommands` (`commands/meta.rs`)

4 tools, but only 3 round-trip through the IPC (the fourth, `get_server_version`, is short-circuited on the server). Sync because the values are already in `PluginState`:

| Tool | Returns |
|------|---------|
| `ping` | `{"message": "pong"}` |
| `get_engine_version` | `{"engine_version": "<major>.<minor>.<patch>"}` |
| `get_plugin_version` | `{"plugin_version": "<crate version>"}` |

`MetaCommands::new()` builds an empty-engine-version instance; `with_engine_version` chains in the value read from `Engine::singleton()` during plugin boot.

## `SceneCommands` (`commands/scene.rs`)

31 tools, all routed through the dispatcher because they all touch Godot APIs. The handler method is a single `match tool { … }` with one arm per tool name; each arm clones `args` and calls `dispatcher.submit(move || cmd_xxx(&a)).await`. The `cmd_*` free functions live below in the same file. See [modules/scene-commands.md](scene-commands.md) for the full catalog and the J↔V conversion rules they rely on.

## The `pipe` helper

`pipe(val: Value) -> Result<Value, String>` (`scene.rs:43`):

```rust
fn pipe(val: Value) -> Result<Value, String> {
    if let Some(e) = val.get("error").and_then(|v| v.as_str()) { Err(e.into()) } else { Ok(val) }
}
```

`cmd_*` functions never return `Result`. They always produce a `Value`; on failure they return `json!({"error": "..."})`. `pipe` lifts that convention into the `Result<Value, String>` shape the routing layer expects, which in turn becomes `IpcResult::Error` on the wire. This keeps `cmd_*` bodies short — no `?`, no `Result` plumbing.

## Adding a new tool — checklist

1. **Pick a name** matching the existing snake_case + verb_object convention (`create_node`, `save_scene`, etc.).
2. **Add the schema** to `crates/server/src/tool_registry.rs::register_defaults` (mind the comments grouping by category).
3. **Add the routing arm** in `crates/gdext/src/commands/scene.rs::handle_scene_tool` *and* the tool name in `TOOL_NAMES: &[&str]` at the top of the same file.
4. **Implement the `cmd_xxx`** free function — see [modules/scene-commands.md](scene-commands.md) for J↔V helpers and `resolve_node`.
5. **Bump the test counts** in `handler.rs::tests` and `tool_registry.rs::tests` (currently 35, 34, 33, 36 etc. — `cargo test` failures pinpoint each location).
6. **Run** `cargo fmt --check --all && cargo clippy --workspace -- -D warnings && cargo build --workspace && cargo test --workspace`. The CI runs exactly that.
7. **Rebuild and redeploy**: `py -3 package_addons.py` (kills the running server, rebuilds both crates, copies the dll into `addons/godot_mcp/bin/`). Restart the MCP client *and* the Godot editor.

Reasons step 6 is mandatory: `clippy` is `-D warnings` and rejects new lints; `cargo build --workspace` ensures both crates type-check together; `cargo test --workspace` is the gate on the count assertions.
