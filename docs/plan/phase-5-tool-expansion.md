# Phase 5 — Tool Group Expansion

> **Status**: ⏳ Not started
> **Estimate**: open (per group: 2-4 days)
> **Depends on**: Phase 4 if we want the new tools usable from HTTP-only clients; otherwise independent

## Why this phase exists

The 125 tools cover scene management, node/property CRUD, GDScript/C# editing, project settings, editor control, input map, plugin management, and search. They don't cover everything else an editor session involves: importing assets, tweaking runtime settings, observing the running scene, or debugging.

This phase grows the catalog along orthogonal axes. Each group is shipped independently with its own page (this file is the index).

## Groups

| Group | Indicative tools | Default state | Notes |
|-------|------------------|---------------|-------|
| `asset_*` | `find_assets`, `import_asset`, `generate_preview`, `create_resource`, `delete_resource` | Enabled | Touches `EditorFileSystem` + `ResourceLoader` + `ResourceSaver`. |
| `script_*` | `read_script_body`, `write_script_body`, `apply_script_patch`, `get_script_diagnostics`, `list_scripts` | Enabled | Sit on top of `ScriptEditor`. Patching is the tricky one — see "Edits via diff" below. |
| `project_*` | `get_project_setting`, `set_project_setting`, `list_autoloads`, `add_autoload`, `remove_autoload`, `set_input_action` | Enabled | `ProjectSettings::singleton()`. Save with `ProjectSettings::save()`. |
| `editor_*` | `play_scene`, `stop_scene`, `screenshot_viewport`, `get_selection`, `set_selection`, `undo`, `redo` | Enabled | `EditorInterface::play_main_scene/stop_playing_scene`. Selection via `EditorSelection`. |
| `debug_*` | `read_runtime_value`, `step_frame`, `pause`, `resume`, `list_remote_nodes` | **Disabled by default** | Requires the running game to be a remote debugger client. Opt-in via the dock toggle. |

## Per-group page contract

Each group gets a file in `docs/plan/phase-5/<group>.md` covering:

1. Concrete tool list with arg/return shapes (write these so they can be copied straight into `tool_registry.rs::register_defaults`).
2. The Godot API entry points (`EditorInterface`, `ResourceSaver`, `ScriptEditor`, etc.).
3. Anything that needs a new dependency, helper, or extension to `j2v`/`v2j`.
4. Per-tool gotchas (especially anything that re-enters the plugin via signals — see [`.repo_wiki/overview/threading-model.md`](../../.repo_wiki/overview/threading-model.md)).
5. Test plan.

Don't author all five sub-pages up front; write each when a group is picked up.

## Shared work across groups

These items are common to every new group and shouldn't be re-derived per page:

### 5.1  Module structure

For each group, create `crates/gdext/src/commands/<group>.rs` modelled on `scene.rs`:

```rust
pub const TOOL_NAMES: &[&str] = &[ /* ... */ ];

pub struct AssetCommands;
impl CommandHandler for AssetCommands { /* trait stubs */ }

impl AssetCommands {
    pub async fn handle_asset_tool(&self, tool: &str, args: &Value, d: &MainThreadDispatcher) -> Result<Value, String> {
        match tool {
            "find_assets" => { let a = args.clone(); pipe(d.submit(move || cmd_find_assets(&a)).await) }
            // ...
        }
    }
}

fn cmd_find_assets(args: &Value) -> Value { /* ... */ }
```

Add `pub mod asset;` to `commands/mod.rs`. Push `Box::new(asset::AssetCommands::new())` into `create_registry()`.

### 5.2  Routing

Extend `IpcWebSocketServer::route_tool_call` (`crates/gdext/src/ipc/ws_server.rs`) with one more `if X.can_handle(tool) { ... }` branch per group. Today there's an "if meta… else if scene…" sequence; it scales fine to 5-6 groups. Beyond that, refactor into a real registry lookup via `tool_names()`.

### 5.3  Schemas

Add entries in `crates/server/src/tool_registry.rs::register_defaults` grouped by category (the file already has `// Scene Management: Read` style comments — keep the convention). Update the test counts in `handler.rs::tests` and `tool_registry.rs::tests`.

### 5.4  Helpers to extend

`j2v` / `v2j` currently handle `Vector2/3/4`, `Color`, `Rect2`, `Quaternion`, `Resource`. Likely additions:

- `Transform2D` / `Transform3D` (matrix shapes)
- `Basis`
- `Callable` (probably out of scope — security risk)
- `Dictionary` ↔ JSON object (already implicit via `Resource` short-circuit, but not general)
- `Array` ↔ JSON array

When extending, **always update both directions** so round-trip stability holds. See [`.repo_wiki/design/decisions.md#adr-007`](../../.repo_wiki/design/decisions.md).

### 5.5  Per-tool enable bit usage

`debug_*` should ship with `enabled: false` in the registry. Add a `register_disabled(name, desc, schema)` helper to `tool_registry.rs` to make the intent explicit.

## Edits via diff (for `script_*`)

`write_script_body(path, content)` is destructive. Better:

- `apply_script_patch(path, search, replace)` — unique substring replacement; fail if `search` matches zero or more than one occurrences. Pattern borrowed from this project's own edit tool.
- `apply_script_patches(path, [{search, replace}, ...])` — atomic batch (all or nothing).

This mirrors how AI clients already deal with code edits and avoids whole-file overwrites.

## Risks

- **Resource save side effects**. Saving a `.tres` triggers `EditorFileSystem` rescan → signals back to plugins. Verify the pump model still holds; if not, batch operations behind a single deferred call.
- **`ScriptEditor` modifies in-memory buffers, not files**. Saving requires `ScriptEditor::save_all_scripts` or similar. Pick a convention and stick with it (suggest: every `apply_script_patch` saves; expose a `save_script(path)` separately if needed).
- **Remote debugger integration is poorly documented**. Budget extra time for `debug_*`; consider deferring to a Phase 5b after the simpler groups land.

## Done means (per group)

- [ ] All tools registered (schema) and routed (`cmd_*`).
- [ ] Live test against an open editor: every tool called at least once with realistic args.
- [ ] Test counts in `handler.rs` and `tool_registry.rs` bumped.
- [ ] Catalog page updated: [`.repo_wiki/reference/tools-catalog.md`](../../.repo_wiki/reference/tools-catalog.md).
- [ ] Log entry in [`.repo_wiki/log.md`](../../.repo_wiki/log.md).
- [ ] The group's `docs/plan/phase-5/<group>.md` deleted; this index updated.
