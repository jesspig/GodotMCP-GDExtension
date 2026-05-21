# `godot-mcp-server`

> The MCP-facing process. Built on `rmcp` 1.7 with the stdio transport. Connects to the GDExtension via a WebSocket client.

## Module map

| File | Role |
|------|------|
| `crates/server/src/main.rs` | clap CLI (`--godot-port`, default 9500). Starts rmcp on stdio. No HTTP path. |
| `crates/server/src/handler.rs` | `GodotMcpHandler`: rmcp `ServerHandler` impl. `get_info`, `list_tools`, `call_tool`. Owns `ToolRegistry` and a lazily-connected `GodotBridge`. |
| `crates/server/src/tool_registry.rs` | `ToolRegistry` over `Arc<DashMap<String, ToolEntry>>`. `register_defaults()` lists all 35 tool schemas. `set_tool_enabled`, `update_from_notification`. |
| `crates/server/src/bridge.rs` | `GodotBridge`: WebSocket client. id → `oneshot::Sender` map, listens for both `IpcResponse` and `IpcNotification` frames. |

## Lifecycle

```
main()                                           handler.rs           bridge.rs
  │                                                  │                    │
  ├─ parse CLI                                       │                    │
  ├─ GodotMcpHandler::new(port)  ──────────────────► registry initialised │
  │  (bridge: Arc<Mutex<Option<…>>>) None             │                    │
  ├─ handler.serve(stdin, stdout)                    │                    │
  │     ↓ rmcp loop                                  │                    │
  │ list_tools  ────────────────────────────────────► reads registry.get_enabled_tools()
  │ call_tool   ────────────────────────────────────► handle_tool_call
  │                                                      ├─ has_tool? enabled?
  │                                                      ├─ "get_server_version" → const string, no bridge
  │                                                      └─ else: forward_tool_call ──────► ensure_bridge() ──► GodotBridge::connect
  │                                                                                            │
  │                                                                                            └─ bridge.call("tool_call", {tool, args})
```

`ensure_bridge` lazily connects on the first non-meta tool call. If the connection drops (`bridge.call` returns `Err`), the handler drops the bridge so the next call retries from scratch.

`get_server_version` is special: it returns `CARGO_PKG_VERSION` synchronously, never opening a bridge. All other tools — including `ping` — round-trip to the GDExtension.

## `handle_tool_call` dispatch

`crates/server/src/handler.rs:81`. The contract:

1. If `!registry.has_tool(name)` → `Err("Unknown tool: <name>")`. MCP returns `-32602 invalid_params`.
2. If `!registry.is_tool_enabled(name)` → `Err("Tool '<name>' is disabled")`.
3. For `get_server_version` → `Ok(CARGO_PKG_VERSION)`.
4. Otherwise wrap the args in `{"tool": name, "args": args}` and call `bridge.call("tool_call", params)`. Return either the JSON-serialised result, the configured offline message (if the bridge can't connect), or the bridge error string.

Per-tool offline messages are picked from a small match (`ping`, `get_engine_version`, `get_plugin_version` get bespoke Chinese strings; everything else falls back to a generic one). They are user-facing in the MCP client, hence the localisation.

`call_tool` (`handler.rs:142`) is just glue: pulls `request.arguments` as a `serde_json::Map`, wraps it in `Value::Object`, calls `handle_tool_call`. **Note**: rmcp's `CallToolRequestParams.arguments` is `Option<Map<String, Value>>`, not `Option<Value>` — the helper conversion is mandatory.

## `ToolRegistry`

A `DashMap<String, ToolEntry>` behind `Arc` so it can be shared across all rmcp request handlers without locks.

- `register_defaults` is one giant `vec![...]` of `(name, description, schema)` tuples. All 35 entries live there; the schema for each is built with a tiny `schema(props, required)` helper that splices a JSON-string properties block into `{"type":"object","properties":{...},"required":[...]}`.
- `set_tool_enabled(name, bool)` flips the enabled flag. `get_enabled_tools()` filters the map for `list_tools`.
- `update_from_notification(&ToolListUpdate)` is called by the bridge when the GDExtension broadcasts `tool_list_updated`. It only flips flags for tools that already exist — unknown names are silently ignored.

**Tests assert exact counts** (currently `total == 35`, `enabled == 35`, plus per-test arithmetic like "after disabling 2, count is 33"). When adding or removing tools, update both `handler.rs::tests` and `tool_registry.rs::tests` together or the whole suite breaks.

## `GodotBridge`

Single-connection WebSocket client. `connect_with_handler(port, Option<Arc<GodotMcpHandler>>)` (`bridge.rs:32`) opens `ws://127.0.0.1:{port}`, splits the stream, and spawns a reader task. The reader does two things in one branch:

1. If a frame parses as `IpcResponse`, look up `response.id` in a `DashMap<String, oneshot::Sender<Value>>`, fire the oneshot with `data` (or `{"error": …}` for `IpcResult::Error`).
2. Otherwise try parsing as `IpcNotification`. If `event == "tool_list_updated"`, deserialise the payload as `ToolListUpdate` and call `handler.update_tools(&update)` (the optional `Arc<GodotMcpHandler>` passed in).

`call(method, params)` (`bridge.rs:107`): generate a UUID, register a oneshot sender in `pending`, send the frame, `await` the receiver. There is no timeout, no retry, no heartbeat — connection death is detected only at the next send attempt.

`godot_ready` notifications are logged but otherwise ignored. The unknown-event branch logs and drops.

## Dependencies that matter

- `rmcp = "=1.7"` with features `server`, `macros`, `schemars`, `transport-io`, `transport-streamable-http-server`. Only `transport-io` is actually used. Don't bump the version without re-checking the `ServerHandler` trait shape.
- `tokio-tungstenite = "0.24"` paired with `tokio = "1"` (`features = ["full"]`).
- `dashmap = "6"` for the lock-free pending-id map.
- `clap = "4"` with `derive`.
- `axum = "0.8"` is declared but unused (left over from the planned HTTP transport).

## Tests

20 tests in `handler.rs` and `tool_registry.rs`. All offline: they assert tool-count arithmetic, registry mutation, and the offline-bridge fallback messages. None of them touch a real Godot editor.
