# Module: `dispatcher`

> Tokio-to-main-thread closure queue. File: `crates/gdext/src/dispatcher.rs`.

## The type

```rust
struct DispatcherJob {
    closure: Box<dyn FnOnce() -> Value + Send>,
    response: oneshot::Sender<Value>,
}

pub struct MainThreadDispatcher {
    queue: Arc<Mutex<VecDeque<DispatcherJob>>>,
}
```

It is `Clone` (the wrapped `Arc` is what's cloned), `Send`+`Sync`, no `Drop` magic. Default impl creates an empty queue.

## API

| Method | Caller | Behaviour |
|--------|--------|-----------|
| `MainThreadDispatcher::new()` / `default()` | `editor_plugin::enter_tree` | Single shared instance per plugin. |
| `submit<F>(&self, f: F) -> impl Future<Output = Value>` (`async`) | Any tokio worker | Boxes the closure, pushes it + a fresh `oneshot::Sender` into the queue, awaits the receiver. Returns `Value::Null` if the sender is dropped before responding. |
| `process_pending(&self)` | Main-thread pump (`Callable::from_fn` on `process_frame`) | `mem::take`s the queue, runs each closure synchronously in submission order, and ships its `Value` back through the oneshot. |

The `&self` on `process_pending` is intentional — the queue is behind `Arc<Mutex<...>>`, no `&mut self` is needed. That matters: it allows the pump callable to invoke it without holding any borrow on the plugin instance (see [threading-model.md](../overview/threading-model.md)).

## Why a oneshot?

Each tool call needs exactly one reply, in the same order it was submitted. `tokio::sync::oneshot` is the cheapest channel that gives us:

- back-pressure-free (always sends immediately because the receiver is awaiting),
- single-use (drops the channel on response),
- `Send` for the closure result.

Submission order is preserved within `process_pending` because `VecDeque::push_back` + drain. **There is no parallelism** on the main thread — closures run serially on the editor's frame.

## Failure modes

| Scenario | Outcome |
|----------|---------|
| Submitter cancels (drops the future) before drain | The job still runs; the oneshot send returns `Err(_)` which is ignored (the result is dropped). |
| Mutex poisoned (a panic inside `process_pending`) | `lock().unwrap()` panics. Acceptable: a panic inside Godot APIs is already fatal. |
| Closure panics | The panic propagates up through `process_pending` → the `Callable` → the editor. Treat closures as `catch_unwind`-free; validate inputs in `cmd_*` and return `{"error": ...}` JSON instead. |
| Pump is never installed | Submissions queue forever; awaits hang. The plugin always installs the pump in `enter_tree`; if it falls back (no SceneTree available) it logs a WARN. |

## Test mechanics

`crates/gdext/src/dispatcher.rs:64+` has four tests. They run on a `#[tokio::test]` runtime and exercise the queue without any Godot interaction:

- `submit_and_process_returns_value` — single submit, manual drain, verify value.
- `multiple_jobs_processed_in_order` — three submits, single drain, FIFO.
- `submit_captures_owned_data` — confirms the closure can capture `String` etc.
- `process_empty_queue_is_noop` — drain with nothing pending.

The tests pattern is `submit → yield_now → process_pending → await`; copy it when writing new ones.

## Common mistakes

- **Capturing `&self`** in a closure passed to `submit` — won't compile because `F: 'static + Send`. Use free functions (`cmd_*`) that take the args as owned values, see `commands/scene.rs`.
- **`await`ing inside the closure** — the closure is `FnOnce() -> Value`, not async. If you need to chain another tool, return a partial result and have the caller invoke the next tool.
- **Calling `process_pending` from `IEditorPlugin::process(&mut self)`** — was the original implementation and it crashed with `bind_mut() failed, already bound` on every tool call. Always drain via the `process_frame` Callable pump. See [overview/threading-model.md](../overview/threading-model.md).
