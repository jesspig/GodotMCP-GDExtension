# Module: `logging`

> Cross-thread log channel that funnels worker-thread log lines into Godot's native console macros on the main thread. File: `crates/gdext/src/logging.rs`.

## Why this exists

Godot's `godot_print!`, `godot_warn!`, `godot_error!` macros are **main-thread only**. Calling them from a tokio worker panics with `attempted to access binding from different thread than main thread; this is UB`. But our tool routing — and therefore every `log_info` we want to emit — runs on tokio workers.

`logging.rs` decouples emission from output:

```
worker thread:                    main thread (every frame):
  log_info(tool, msg)               drain_to_console()
        │                                  ▲
        ├─► eprintln!(...)                 │
        │                                  │
        └─► SENDER.send(LogRecord) ────────► RECEIVER.try_iter() ─► godot_print!/warn!/error!
```

## Public API

| Function | Thread | What it does |
|----------|--------|--------------|
| `log_info(tool, msg)` | any | `eprintln!` the line, push `LogRecord` onto the global mpsc. |
| `log_warn(tool, msg)` | any | Same, level `Warn`. (`#[allow(dead_code)]` — no caller today, kept for future use.) |
| `log_error(tool, msg)` | any | Same, level `Error`. |
| `drain_to_console()` | **main thread only** | Pulls every pending `LogRecord` via `try_iter()` and forwards to the matching `godot_*` macro. Called by the pump every frame; also called once during `exit_tree` to flush shutdown messages. |
| `log_info_main` / `log_warn_main` / `log_error_main` | main thread only | Direct `godot_*` calls without going through the channel. Currently unused but reserved for code that already runs on the main thread (dock signal handlers, etc.). |

All output follows the exact format `[Godot MCP][<tool>][<level>] <message>` — including the mirror `eprintln!`. Don't tweak it without coordinating; humans read it in the Godot Output panel and expect it.

## Implementation notes

- `MAX_PAYLOAD_LEN = 512` chars. Longer messages are truncated to the first 512 chars and suffixed with `… (<bytes> bytes total)`. Stops a single noisy log from filling the Output panel.
- The channel is created lazily in `channels()` via `OnceLock`. The first caller to either send a message or drain becomes the initialiser. The `SENDER` and `RECEIVER` halves are stored separately (sender in a `OnceLock<Sender<...>>`, receiver wrapped in `Mutex` inside another `OnceLock`).
- `try_iter()` is non-blocking and drops out when the queue is empty — perfect for a per-frame call that must not stall the editor.
- The channel is unbounded. A misbehaving worker that spams logs will grow memory until the next frame. There is no backpressure; if you need to log inside a hot loop, throttle on the caller side.

## eprintln mirror

Each `emit` writes to stderr first, then to the channel. Two reasons:

1. Plugin boot. The pump isn't installed until `enter_tree` finishes; any log emitted before that lives only on stderr.
2. Headless/test runs. `cargo test` has no Godot runtime; stderr is the only sink available.

Godot does capture subprocess stderr but its main "Output" panel only shows native macro output — that's why the channel exists.

## Caller pattern

`crates/gdext/src/ipc/ws_server.rs:209` — `route_tool_call` logs every request and every response:

```rust
log_info(tool, &format!("called args={}", args));
let result = if meta.can_handle(tool) { … } else if scene.can_handle(tool) { … } else { … };
match &result {
    Ok(v) => log_info(tool, &format!("ok result={}", v)),
    Err(e) => log_error(tool, &format!("failed: {}", e)),
}
```

This is the single source of tool-call observability. Don't sprinkle logs inside `cmd_*` functions; the routing layer covers the entry/exit and that's what users read.

## Failure modes

| Scenario | Behaviour |
|----------|-----------|
| Drain called before any sender (very early boot) | `RECEIVER.get()` returns `None`, drain is a no-op. |
| Mutex on receiver poisoned | Drain returns silently. We tolerate this because logging must never crash the editor. |
| Sender drops (impossible today — it's a `OnceLock`) | Future `try_iter` would observe disconnect. Not a current concern. |
| Receiver lags | Records buffer in the channel. Memory pressure only.

## Related

- [overview/threading-model.md](../overview/threading-model.md) — why the channel and pump exist.
- [modules/editor-plugin.md](../modules/editor-plugin.md) — where `drain_to_console` is wired into the pump.
