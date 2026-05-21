# Module: `editor-plugin`

> The `McpEditorPlugin` GDExtension class. File: `crates/gdext/src/editor_plugin.rs`.

## Class declaration

```rust
#[derive(GodotClass)]
#[class(tool, base=EditorPlugin)]
pub struct McpEditorPlugin {
    #[base]                          base: Base<EditorPlugin>,
    runtime:        Option<tokio::runtime::Runtime>,
    shutdown:       Option<Arc<Notify>>,
    dispatcher:     Option<MainThreadDispatcher>,
    broadcast_tx:   Option<broadcast::Sender<String>>,
    main_dock:      Option<Gd<VBoxContainer>>,
    pump_callable:  Option<Callable>,
    scene_tree:     Option<Gd<SceneTree>>,
}
```

`#[class(tool, base=EditorPlugin)]` means the class also runs in the editor (not just at game runtime) — required for any GDExtension that adds editor UI or hooks into the editor lifecycle.

## `IEditorPlugin` impl — what we override

| Method | Notes |
|--------|-------|
| `init(base)` | Returns the struct with every `Option` set to `None`. No expensive work here. |
| `enter_tree(&mut self)` | Boots the tokio runtime, WS server, dispatcher, log pump, and dock. See below. |
| `exit_tree(&mut self)` | Tears everything down in reverse order. Also calls `logging::drain_to_console()` one last time so shutdown logs reach the editor before the runtime stops. |

**There is no `process(&mut self)` override.** The previous implementation drained the dispatcher there and panicked with `bind_mut() failed, already bound` on every tool call (gdext issue #338 — see [overview/threading-model.md](../overview/threading-model.md)). The drain now runs from a `Callable` connected to `SceneTree::process_frame`, in a call stack that does not hold a borrow on the plugin.

## `enter_tree` sequence

1. `godot_print!("[Godot MCP] Plugin entering tree...")` — main-thread, direct macro call is safe.
2. Read engine version via `Engine::singleton().get_version_info()` → `"{major}.{minor}.{patch}"`.
3. Build a `tokio::runtime::Builder::new_multi_thread().worker_threads(2).enable_all().build()`. If this fails, log and bail — the plugin survives but does nothing.
4. Create `PluginState { engine_version, plugin_version }`, `MainThreadDispatcher`, `Arc<Notify>` shutdown signal.
5. `IpcWebSocketServer::new(9500, state, shutdown, dispatcher.clone(), registry)`. Stash its `broadcast_tx()` for later use by the dock.
6. `runtime.spawn(async move { server.run().await })`. The runtime is moved into `self.runtime`.
7. `install_main_thread_pump(dispatcher)` — wires `Callable::from_fn("godot_mcp_pump", …)` to `SceneTree::process_frame`.
8. Create the dock via `dock::main_dock::create_dock(broadcast_tx.clone())`, add it to `DockSlot::RIGHT_UL`.
9. `godot_print!("[Godot MCP] Plugin loaded!")`.

The pump installs **before** the dock so any log produced by dock setup is flushed promptly.

## `exit_tree` sequence

1. `uninstall_main_thread_pump()` — `disconnect` the Callable from `process_frame` if `is_connected`. Drops `pump_callable` and `scene_tree`.
2. `remove_control_from_docks(dock)` + `dock.free()`.
3. `shutdown.notify_one()` → tokio task observes via `Notify::notified()` and breaks out of the accept loop. Sleep 200 ms so the server has time to shut down cleanly.
4. Drop dispatcher and runtime.
5. Final `logging::drain_to_console()`.

The 200 ms sleep is deliberately synchronous on the main thread (`std::thread::sleep`) — `exit_tree` already runs at editor shutdown, so blocking for a fifth of a second is acceptable.

## The pump

`install_main_thread_pump` (`editor_plugin.rs` near the bottom):

```rust
let mut tree = self.base().get_tree_or_null()?;
let callable = Callable::from_fn("godot_mcp_pump", move |_args| {
    dispatcher.process_pending();
    logging::drain_to_console();
    Variant::nil()
});
tree.connect("process_frame", &callable);
self.pump_callable = Some(callable);
self.scene_tree = Some(tree);
```

`Callable::from_fn` is single-threaded; the closure can only be invoked from the same thread that created it (the editor main thread). The closure captures the dispatcher by move; since `MainThreadDispatcher` clones via `Arc`, every clone of the callable still shares the same queue.

Why `process_frame` and not `_process(&mut self)`? Because `process_frame` is a SceneTree signal: when Godot fires it, no `Gd<McpEditorPlugin>` is currently bound. Closures invoked here can call `EditorInterface::save_scene()` etc. without re-entering the plugin (`scene_changed` signals coming back are also dispatched out-of-stack).

`uninstall_main_thread_pump` checks `is_connected` before `disconnect` — Godot panics on disconnecting a never-connected signal, so the guard is mandatory.

## `broadcast_tool_list_updated`

A helper that lets dock UI handlers (or future tool-manifest changes) push `tool_list_updated` notifications back to every connected WS client. Currently used only by `dock::tool_manager` and marked `#[allow(dead_code)]` for the outer `McpEditorPlugin` method that's a thin wrapper.

## State invariants

All fields are `Option<...>` because they're populated in `enter_tree` and torn down in `exit_tree`. Outside that window, treat them as `None`. The `Default` impl is generated implicitly by `init` constructing the all-`None` struct.

## Common mistakes

- **Overriding `_process(&mut self)`** — see history above; do not. Anything per-frame goes through the pump callable.
- **Calling `self.base_mut()` inside the pump closure** — the closure captures by `move`, no `self` reference. Stick to free APIs (`EditorInterface::singleton()`, etc.).
- **Re-installing the pump without uninstalling** — `connect` would queue a second copy of the callable, so the dispatcher would drain twice per frame. The flow is `uninstall → install` only at the boundaries of `enter_tree` / `exit_tree`.
