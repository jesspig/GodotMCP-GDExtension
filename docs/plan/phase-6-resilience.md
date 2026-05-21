# Phase 6 — Resilience

> **Status**: ⏳ Not started
> **Estimate**: 1 week
> **Depends on**: best done after Phase 5 — the failure modes only matter under sustained, varied use

## Why this phase exists

Today the IPC bridge is intentionally minimal: no heartbeat, no timeouts, no reconnect schedule, no shutdown handling. That's *fine* during early development — failures fail loudly, and a quick "kill it and restart" loop is faster than building correct retry logic. Once people are using the server in real sessions, the long-tail glitches start mattering:

- A network hiccup (sleep/wake on Mac, Wi-Fi roam) silently half-opens the WS. The next call hangs.
- A misbehaving tool blocks the dispatcher; the AI client waits forever.
- Two MCP clients connect simultaneously — works today, but no test covers it.
- Ctrl-C the server doesn't drain in-flight rmcp calls cleanly.
- After-the-fact debugging is stderr-only; no log file.

This phase is the "production hardening" pass.

## What to add

### 6.1  Heartbeat

`crates/server/src/bridge.rs::GodotBridge`: every 10s, send a `{"id":"<uuid>","method":"ping","params":{}}` and require a reply within 3s. On miss → drop the bridge (the existing lazy-reconnect handles the rest).

`crates/gdext/src/ipc/ws_server.rs`: nothing to add — `ping` is already a meta tool that returns instantly. The heartbeat just reuses it.

**Pitfall**: the heartbeat must not consume the `pending` slot for *real* user calls. Use a separate task with its own oneshot.

### 6.2  Per-call timeout

`GodotBridge::call` today:

```rust
let result = rx.await?;
```

Wrap in `tokio::time::timeout`:

```rust
let result = tokio::time::timeout(TIMEOUT, rx).await??;
```

Default `TIMEOUT = Duration::from_secs(30)`. Configurable via a server CLI flag `--tool-timeout 60`. On timeout: clear the pending entry (no leak) and return an error message that the MCP client surfaces.

**Pitfall**: a slow tool (e.g. `save_all_scenes` on a large project) is legitimate. 30s is a starting point; revisit after a few weeks of usage.

### 6.3  Multi-client correctness

Today `IpcWebSocketServer::handle_connection` spawns a task per WS client. Requests/responses use `id` to correlate, so concurrent clients *should* work — but no test enforces it. Add:

- Integration test: open two `GodotBridge` instances simultaneously, fire interleaved calls, assert each gets its own response.
- Validate that `broadcast::Sender` notifications (e.g. `tool_list_updated`) reach all subscribers within the 64-slot capacity.

If the dock toggle of a single tool fires 100 times in a frame (slider-style), older subscribers might lag past 64 and drop messages. Sit on it; if it becomes real, switch to `tokio::sync::watch` for state.

### 6.4  Audit / JSONL log

Add an optional `--log-file <path>` CLI flag to the server. When set: every `[Godot MCP]` log line is also written as a JSON record to the file. Schema:

```jsonc
{ "ts": "2026-05-22T01:23:45.678Z", "tool": "create_node", "level": "INFO", "msg": "called args={...}", "duration_ms": 12 }
```

The duration field needs a wrapper around `bridge.call` — start a timer before send, stop on response. Reuse the existing `crates/gdext/src/logging.rs` channel API; add a `log_with_timing(tool, args, fut)` helper on the server side.

### 6.5  Graceful shutdown

Today's `tokio::main` returns when `service.waiting()` returns. There's no SIGINT handler.

```rust
let shutdown = async {
    tokio::signal::ctrl_c().await.ok();
    eprintln!("[MCP Server] Received Ctrl-C, shutting down...");
};

tokio::select! {
    res = service.waiting() => res?,
    _ = shutdown => {}
}
```

Plus, drop the bridge explicitly so it sends a WS close frame to the editor (cleaner than the editor side seeing a TCP RST).

### 6.6  Reconnect schedule on the editor side

Today `IpcWebSocketServer` just accepts. If the server side socket gets killed and restarts, the editor will accept the new connection automatically — that's fine.

What we *don't* have: a way for the editor to actively probe "is anyone still there?" That's not strictly required because the dispatcher works regardless. Skip unless it becomes a real complaint.

## Things explicitly out of scope

- **TLS / auth**. Loopback assumption holds. If we ever expose over LAN (would be a Phase 7), revisit.
- **Cross-process state recovery after a Godot crash**. The server can keep running; the editor's state is gone anyway. Just retry.

## Tests to add

- `bridge.rs`: heartbeat sends correctly; missed heartbeat drops the bridge; timeout fires after `TIMEOUT`.
- `bridge.rs`: two concurrent clients each get their own responses (need a mock WS server).
- `main.rs`: SIGINT triggers graceful shutdown within 2s.

## Done means

- [ ] Heartbeat + per-call timeout shipped; failures result in actionable error messages.
- [ ] Multi-client integration test green.
- [ ] `--log-file` flag works; sample audit log shipped in `docs/plan/samples/` (or wherever).
- [ ] Ctrl-C shutdown finishes cleanly without rmcp panics.
- [ ] [`.repo_wiki/modules/ipc-bridge.md`](../../.repo_wiki/modules/ipc-bridge.md) updated; the "Things this module deliberately does not do" list shrinks.
- [ ] Log entry in [`.repo_wiki/log.md`](../../.repo_wiki/log.md).
- [ ] This file deleted.
