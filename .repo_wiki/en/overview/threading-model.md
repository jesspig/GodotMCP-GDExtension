# Threading Model

> **Must read before touching gdext.** This is the single most error-prone part of the project.

## Core Problem

Tool routing runs on tokio worker threads. **Almost every Godot API panics if called off the main thread.**

The project has two mechanisms to handle this — and why `EditorPlugin::process()` can't be used directly.

## Mechanism 1: MainThreadDispatcher

```mermaid
flowchart LR
    subgraph Tokio["tokio Worker Thread"]
        T1["route_tool_call()"]
        T2["submit(|| cmd_*())"]
    end
    
    subgraph Queue["VecDeque<DispatcherJob>"]
        Q1["(closure, oneshot Sender)"]
        Q2["(closure, oneshot Sender)"]
    end
    
    subgraph Main["Godot Main Thread"]
        M1["process_frame pump"]
        M2["process_pending()"]
        M3["Execute closure → Godot API"]
    end
    
    T1 -->|clone args| T2
    T2 -->|push_back| Q1
    T2 -->|push_back| Q2
    M1 -->|every frame| M2
    M2 -->|drain| Q1
    M2 -->|drain| Q2
    Q1 -->|run| M3
    Q2 -->|run| M3
    M3 -->|oneshot.send| T1
```

- Worker thread calls `dispatcher.submit(move || { /* Godot API */ })`, gets back a `oneshot` future
- Submitted as `Box<dyn FnOnce() -> Value + Send>` closure, pushed to `VecDeque`
- Main thread calls `process_pending()` to pop and execute all queued closures
- All `cmd_*` functions go through this, without exception

## Mechanism 2: Cross-Thread Logging

```mermaid
flowchart LR
    subgraph Worker["tokio Worker Thread"]
        W["log_info()"]
    end
    
    subgraph Channel["mpsc channel"]
        C1["(INFO, tool, msg)"]
        C2["(WARN, tool, msg)"]
    end
    
    subgraph Main["Main Thread"]
        M["drain_to_console()"]
        O["godot_print! / godot_warn!"]
    end
    
    W -->|send| C1
    W -->|send| C2
    M -->|drain try_iter| C1
    M -->|drain try_iter| C2
    C1 --> O
    C2 --> O
    
    W2["eprintln! (mirror)"] -.->|fallback| Terminal["terminal stderr"]
```

- Worker calls `log_info/log_warn/log_error` → message enters mpsc channel + `eprintln!` mirror
- Main thread calls `drain_to_console()` → forwards to `godot_print!`/`godot_warn!`/`godot_error!`
- **Never call `godot_print!` from a tokio worker thread.**

## Why Not `EditorPlugin::process()` (bind_mut Trap)

```
EditorPlugin::process(&mut self)  ← holds exclusive borrow (bind_mut)
  └─ calls some Godot API
      └─ that API synchronously triggers a signal
          └─ signal callback tries to access the editor plugin
              └─ Gd<T>::bind_mut() crashes: "already bound"
```

Both queues are pumped via `Callable::from_fn` connected to `SceneTree::process_frame`:

```rust
// editor_plugin.rs (simplified)
let callable = Callable::from_fn("godot_mcp_pump", move |_args| {
    dispatcher.process_pending();
    logging::drain_to_console();
    Variant::nil()
});
tree.connect("process_frame", &callable);
```

This operates outside any `McpEditorPlugin` borrow, so Godot API calls are safely re-entrant.

## Adding a New Tool: Rules

1. Add routing branch in `ws_server.rs::route_tool_call`
2. That branch calls `d.submit(move || cmd_your_tool(&a)).await`
3. Actual `cmd_your_tool` runs on main thread via dispatcher
4. Wrap return with `pipe()`: `pipe(d.submit(...).await)` converts `json!({"error": "..."})` to `Err`
