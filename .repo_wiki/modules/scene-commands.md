# Module: `scene-commands`

> The 31 Godot-touching tools. File: `crates/gdext/src/commands/scene.rs` (≈ 700 lines).

## Anatomy

| Section | Lines (approx.) | Contents |
|---------|-----------------|----------|
| `TOOL_NAMES` constant | 12–22 | Hardcoded list of all 31 scene-tool names. `can_handle` checks membership. |
| `SceneCommands` impl | 24–84 | The async router. One match arm per tool: `let a = args.clone(); pipe(d.submit(move \|\| cmd_xxx(&a)).await)`. |
| Helpers | 86–225 | `s(args, key)`, `resolve_node`, `v2j`, `j2v`, `obj_keys_match`, `obj_get_f32`. |
| `cmd_*` implementations | 230+ | Free functions, one per tool. Take `&Value`, return `Value`. Never `Result`. |
| Tree serialiser | end of file | `serialize_tree` used by `cmd_get_scene_tree`. |

## Why every `cmd_*` is a free function

`handle_scene_tool` closures are passed to `dispatcher.submit(move || …)` which requires `F: FnOnce() -> Value + Send + 'static`. Capturing `&self` is impossible — there's no lifetime that satisfies `'static`. The pattern is: clone `args` (`let a = args.clone();`), then `move` it into the closure that calls a free function. **Do not** make `cmd_*` methods on `SceneCommands`.

## Error reporting convention

`cmd_*` functions never return `Result`. They always produce a `Value`. On failure they return `json!({"error": "human-readable message"})`. The `pipe` helper in the routing layer detects the `"error"` key and converts it into `Err(String)`, which the IPC layer turns into `IpcResult::Error`.

Why: keeps function bodies short (no `?`, no try-conversions for JSON), and lets the same `Value` carry either a success payload or an error message.

```rust
fn cmd_rename_node(args: &Value) -> Value {
    let p = s(args, "node_path");
    let new_name = s(args, "new_name");
    let ei = EditorInterface::singleton();
    let root = match ei.get_edited_scene_root() {
        Some(r) => r,
        None => return json!({"error": "No scene open"}),
    };
    match resolve_node(&root, p.as_str()) {
        Some(mut n) => {
            n.set_name(&StringName::from(new_name.as_str()));
            json!({"new_path": n.get_path().to_string()})
        }
        None => json!({"error": format!("Node not found: {}", p)}),
    }
}
```

## `resolve_node(root, path)` — the only correct node lookup

```rust
fn resolve_node(root: &Gd<Node>, path: &str) -> Option<Gd<Node>>
```

Accepts any of: `""`, `"."`, `"/"`, `"/root"`, the root's exact name, `"/root/<root-name>"`, or a normal `NodePath` (relative to `root`). For the first batch it returns the root itself; otherwise it falls through to `root.get_node_or_null(path)`.

**Always use `resolve_node` instead of `root.get_node_or_null` directly.** Tools that target the scene root (e.g. `instantiate_scene` with no parent_path) crashed with "Parent not found" before this helper existed. The crash showed up only in the multi-tool integration test because most callers pass a real subpath.

## `v2j` — Variant → JSON

`crates/gdext/src/commands/scene.rs:106`. Tries each conversion in order and emits a typed JSON shape, falling back to `Variant::to_string` for unknown types:

| Source Variant | JSON shape |
|----------------|-----------|
| `nil` | `null` |
| `bool` | `true` / `false` |
| `i64` | integer |
| `f64` | number |
| `GString` | string |
| `Vector2` | `{"x": …, "y": …}` |
| `Vector3` | `{"x": …, "y": …, "z": …}` |
| `Vector4` | `{"x": …, "y": …, "z": …, "w": …}` |
| `Color` | `{"r": …, "g": …, "b": …, "a": …}` |
| `Rect2` | `{"position": {"x": …, "y": …}, "size": {"x": …, "y": …}}` |
| `Quaternion` | `{"x": …, "y": …, "z": …, "w": …}` |
| `Gd<Resource>` with non-empty path | `{"resource_path": "res://..."}` |
| everything else | `Variant::to_string()` as a JSON string |

This shape is the canonical "round-trip" format — anything `v2j` emits, `j2v` will accept and reconstruct.

## `j2v` — JSON → Variant

`crates/gdext/src/commands/scene.rs:142`. Type detection is by shape:

| Input JSON | Result Variant |
|------------|---------------|
| `null` | `Variant::nil()` |
| `true` / `false` | bool |
| integer | `i64` |
| float | `f64` |
| string starting with `res://` or `user://` | tries `try_load::<Resource>(path)` → `Variant::from(Gd<Resource>)`; falls back to `GString` on failure |
| other string | `GString` |
| object with exactly `{x, y}` | `Vector2::new(x, y)` |
| object with exactly `{x, y, z}` | `Vector3::new(x, y, z)` |
| object with exactly `{x, y, z, w}` | `Quaternion::new(x, y, z, w)` (not Vector4! — there's no Vector4 detection) |
| object with exactly `{r, g, b, a}` | `Color::from_rgba(r, g, b, a)` |
| object with exactly `{r, g, b}` | `Color::from_rgb(r, g, b)` |
| object with exactly `{position, size}` (each a `{x, y}` map) | `Rect2::from_components(px, py, sx, sy)` |
| object with `{"resource_path": "res://..."}` | `try_load::<Resource>(path)` |
| any other object | `GString::from(serde_json::to_string(v))` — the JSON text, as a string. Almost certainly wrong; surfaces as Godot rejecting the assignment. |
| array of 2/3/4 numbers | `Vector2`/`Vector3`/`Color` respectively |
| other array | `GString` of the JSON text |

### Why this matters: the silent-corruption bug

Before `j2v` had typed detection, passing `{"x": 100, "y": 200}` to `set_property("position", …)` fell through to the "stringify" branch. Godot accepted the string, failed to convert it to `Vector2`, and stored `(1e-05, 1e-05)` (its near-zero fallback). The tool returned success. The scene quietly broke.

The lesson: **anything that mutates a Variant on a Godot object must go through `j2v`**, and when adding a new Variant type, extend **both** `v2j` and `j2v` together so the round-trip property holds.

## Resource loading — always `try_load`

`use godot::tools::try_load;`. Returns `Result<Gd<T>, IoError>` where `T: Inherits<Resource>`.

- Prefer `try_load::<PackedScene>(path)` over `ResourceLoader::singleton().load(path).cast::<PackedScene>()`. The latter takes 1 arg in gdext 0.5 (not 2), is more verbose, and gives worse error messages.
- For scripts: `try_load::<godot::classes::Script>(path)`.
- For any `Resource`: `try_load::<Resource>(path)`.

`j2v` uses `try_load::<Resource>` so any subclass round-trips.

## Scene-file operations — through `EditorInterface`, never the disk

`open_scene`, `save_scene`, `save_scene_as`, `save_all_scenes`, `close_scene`, `reload_scene`, `get_open_scenes`, `get_open_scene_roots`, `mark_scene_unsaved` all go through `EditorInterface::singleton()`. **Never write `.tscn` files directly** — the editor won't see the changes until the next reimport scan, the in-memory tree won't update, and dependent panels (FileSystem, Scene tab) won't refresh.

For "save a node subtree as a new .tscn" use `branch_to_scene` (which uses `PackedScene::pack(node)` + `godot::tools::save(packed, path)`). For the inverse use `scene_to_branch` (which calls `try_load::<PackedScene>(path).instantiate()` and swaps the result in place).

`open_scene` uses the builder form: `ei.open_scene_from_path_ex(&GString::from(path)).set_inherited(false).done()`. Same pattern for `save_scene_as_ex().with_preview(true).done()`.

## Scene-root special case for `set_as_root`

`tree.set_edited_scene_root(&node)` lives on `SceneTree`, not on `EditorInterface`. `node.get_tree()` returns `Gd<SceneTree>` directly (not `Option`), so it can be chained.

## Notable individual commands

| Tool | Notes |
|------|-------|
| `get_scene_tree` | Walks the tree via `serialize_tree(node, depth, max_depth)`. Each node serialised as `{name, type, path, child_count, children: [...]}`. Default `max_depth = 10`. |
| `create_node` | Uses `ClassDb::singleton().instantiate(node_type)` which returns `Variant`; needs `.try_to::<Gd<Node>>()` to extract a typed node. After creation: `parent.add_child(&new)`; `new.set_owner(&root)` so it's saved with the scene. |
| `set_property` | Always routes the value through `j2v`. The string-only legacy path is gone. |
| `attach_script` | Uses `node.set("script", Variant::from(script))` instead of `node.set_script(Some(script))` because gdext 0.5's `set_script` has a `Pass = ByValue` constraint that mismatches `Option<Gd<Script>>`. |
| `duplicate_node` | `node.duplicate_node_ex().done_or_null()` (gdext 0.5 form — there is no `try_duplicate`). |
| `branch_to_scene` | `PackedScene::new_gd().pack(&node)` + `godot::tools::save(packed, path)` (1-arg form, not `ResourceSaver::singleton().save(packed, path)` which takes 2). |
| `set_as_root` | `node.get_tree().set_edited_scene_root(&node)`. Marks the scene unsaved as a side effect. |

## Don't add these patterns

- Direct `.tscn` parsing / writing.
- `parking_lot` or any locking inside a `cmd_*`; the dispatcher already serialises them.
- `tokio::spawn` from inside a `cmd_*`; you're already on the main thread.
- Calling `dispatcher.submit` recursively from a `cmd_*`; it deadlocks (the pump can't drain while the current closure is running).
