# Threading Model

> The single most important page in this wiki. Almost every bug in `crates/gdext/` traces back to violating one of the rules here.

## The two worlds

```
┌────────────────────────────────────────────┐      ┌─────────────────────────────────────────────┐
│ tokio runtime (2 worker threads)           │      │ Godot editor main thread                    │
│                                            │      │                                             │
│  • IpcWebSocketServer::run                 │      │  • McpEditorPlugin lifecycle                │
│  • handle_connection / handle_request      │      │  • EditorInterface / SceneTree / Node calls │
│  • route_tool_call (async)                 │      │  • godot_print! / godot_warn! / godot_error!│
│  • SceneCommands::handle_scene_tool        │      │  • Dock UI (signals fire here)              │
│  • dispatcher.submit().await               │      │  • dispatcher.process_pending()             │
│  • mpsc::Sender (log) push                 │      │  • logging::drain_to_console()              │
│                                            │      │                                             │
│  ⛔ NEVER touch any godot:: API here       │      │  ⛔ NEVER block; never call .await here     │
└────────────────────────────────────────────┘      └─────────────────────────────────────────────┘
            │                                                              ▲
            │  closures (FnOnce → Value)                                   │
            └──────────► MainThreadDispatcher (Arc<Mutex<VecDeque>>) ──────┤
            │                                                              │
            │  LogRecord                                                   │
            └──────────► mpsc::Sender → channel → mpsc::Receiver ──────────┤
                                                                           │
                                            drained every frame by ───────►│
                                            Callable connected to SceneTree::process_frame
```

## Why this matters: two failure modes

### 1. Calling Godot APIs from a worker thread

```
panic: attempted to access binding from different thread than main thread;
       this is UB - use the "experimental-threads" feature
```

`Gd<T>` is not thread-safe (without the `experimental-threads` feature, which we do not enable). Any direct `EditorInterface::singleton()` call from a tokio task aborts the editor. This is why `cmd_*` functions in `scene.rs` are wrapped in `dispatcher.submit(move || cmd_xxx(&args)).await`.

### 2. Executing closures while the plugin is bind_mut'd

```
panic: Gd<T>::bind_mut() failed, already bound;
       T = godot_mcp_gdext::editor_plugin::McpEditorPlugin
       cannot borrow while accessible mutable borrow exists
```

This was the root cause of the 35-tool test-run crash in the project history. The mechanism (per gdext issue #338):

- When Godot calls a trait method like `IEditorPlugin::process(&mut self)`, gdext implicitly `bind_mut()`s the surrounding `Gd<Self>` for the whole call.
- If a `cmd_*` closure invokes an `EditorInterface` API that synchronously fires a Godot signal (e.g. `scene_changed`), Godot dispatches the signal back into the plugin → second `bind_mut` attempt → panic.

So **we deliberately do NOT execute closures inside `process(&mut self)`**. The plugin's `process` is intentionally empty (see `editor_plugin.rs` — there is no `fn process` override). Instead, a `Callable::from_fn` connected to `SceneTree::process_frame` runs the drain logic. That callback has no `bind` on `McpEditorPlugin` in its call stack, so re-entrant signals are safe.

## The pump

Installed in `McpEditorPlugin::install_main_thread_pump` (`crates/gdext/src/editor_plugin.rs`):

```rust
let callable = Callable::from_fn("godot_mcp_pump", move |_args| {
    dispatcher.process_pending();
    logging::drain_to_console();
    Variant::nil()
});
tree.connect("process_frame", &callable);
```

`pump_callable` and `scene_tree` are stored on the plugin so `uninstall_main_thread_pump` can `disconnect` cleanly in `exit_tree`.

**Rules of engagement**:

| Place | Allowed |
|-------|---------|
| Inside `enter_tree`/`exit_tree` (`&mut self`) | Direct Godot API. Just don't call long-running closures here. |
| Inside `cmd_*` (free fns invoked by pump) | Direct Godot API. Plugin is not bound. |
| Inside any tokio task (worker thread) | Submit a closure via `dispatcher.submit(...).await`. Push log records via `logging::log_info/warn/error`. **Nothing else.** |
| Inside a `Callable` connected to a Godot signal | Direct Godot API; treat as main-thread code. |

## Logging — same problem, same shape

Godot's `godot_print!`, `godot_warn!`, `godot_error!` are also main-thread-only (they reach into the engine binding internally). The first iteration tried to use them directly from tokio workers and crashed the editor every time a tool was logged.

`crates/gdext/src/logging.rs` solves it with the same pattern as the dispatcher:

- Worker thread calls `log_info(tool, msg)`. The implementation:
  - `eprintln!`s the line so it shows up on stderr immediately (useful when launching Godot from a shell).
  - Pushes a `LogRecord` onto a global `mpsc::Sender`.
- Main thread (via the pump) calls `drain_to_console()` which pulls every queued record from the receiver and forwards it to `godot_print!` / `godot_warn!` / `godot_error!`.

All log lines follow `[Godot MCP][<tool>][<level>] <message>` exactly. The format is asserted by humans against the Godot Output panel; do not "improve" it without coordinating.

Convenience variants `log_info_main` / `log_warn_main` / `log_error_main` are also provided for code that runs on the main thread directly (e.g. dock signal handlers) and prefers Godot's styled output. They are currently unused but kept under `#[allow(dead_code)]`.

## Tests can't load the Godot runtime

`godot_print!` and friends panic outside the editor. None of our unit tests touch them; the dispatcher tests only check the queue mechanics. Keep new `#[cfg(test)]` blocks Godot-free.

## Symptoms → likely cause cheat sheet

| Editor panic / log | Likely cause |
|-----|------|
| `attempted to access binding from different thread than main thread` | A `godot::` API call leaked outside `dispatcher.submit` / the pump |
| `Gd<T>::bind_mut() failed, already bound; T = McpEditorPlugin` | A closure ran inside `process(&mut self)` or another `&mut self` callback while Godot re-entered the plugin via a signal |
| Tool returns `Godot 编辑器未连接` | Server can't reach the WS (`ws://127.0.0.1:9500`). Plugin not loaded, or port mismatch |
| Tool returns `Tool '...' handler not yet implemented` | Schema registered in `tool_registry.rs` but no routing arm in `commands/scene.rs::handle_scene_tool` |
| MCP client reports `Unknown tool: ...` | Schema not in `tool_registry.rs::register_defaults` (the registry decides what `list_tools` returns) |

See also:
- [modules/dispatcher.md](../modules/dispatcher.md) — internals of the closure queue
- [modules/logging.md](../modules/logging.md) — internals of the log channel
- [modules/editor-plugin.md](../modules/editor-plugin.md) — pump install / uninstall lifecycle
