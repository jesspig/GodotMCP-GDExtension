# GodotMCP — Agent Instructions

MCP server that exposes the Godot 4.6+ editor to AI tools via **99 commands** (96 gdext + 3 server-side editor control).

## Architecture

```
AI client ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                       (crates/server, bin)                         (crates/gdext, cdylib)
                                  │                                          │
                                  └──── crates/core ── shared protocol types ┘
```

- **stdio is the only MCP transport**. `transport-streamable-http-server` is in deps but unwired.
- **Adding a tool requires both sides**: register schema in `crates/server/src/tool_registry.rs::register_defaults` AND add a routing arm in `crates/gdext/src/ipc/ws_server.rs::route_tool_call`. Server-side tools only need the registry + `handler.rs` match arm.
- **Hardcoded counts in tests**: `tool_registry.rs` + `handler.rs` assert `total == 99`. Update both when adding/removing tools.
- **`crates/gdext/src/commands/mod.rs::create_registry()`** (8 groups for name discovery) and **`route_tool_call`** (13 routing groups) must stay in sync — `SceneCommands`, `ScriptHelpersCommands`, `SearchCommands`, `UndoCommands`, `ProjectSettingsCommands` exist in routing but NOT in `create_registry()`.

## Handler routing chain (13 groups)

```
server handler → EditorControl (3, server-side)
gdext route_tool_call → MetaCommands(4) → NodeCommands(17) → PropertyCommands(19)
  → CollisionCommands(2) → FindCommands(4) → ScriptHelpersCommands(3)
  → ProjectSettingsCommands(3) → SceneCommands(15) → ScriptGdCommands(5)
  → ScriptCsCommands(6) → SearchCommands(3) → UndoCommands(2)
  → Property3dCommands(6) → NodeConvenience(4) → SceneInfo(1)
```

## Main-thread / tokio split (biggest gotcha)

Tool routing runs on tokio worker threads. **Almost every Godot API panics if called off the main thread.**

1. **`MainThreadDispatcher`**: worker calls `dispatcher.submit(move || cmd_*(...))` → returns a `oneshot` future. Main thread drains `VecDeque` via `process_pending()`. **All `cmd_*` functions are invoked through this.**
2. **`logging` module**: worker calls `log_info/log_warn/log_error` → mpsc channel + `eprintln!`. Main thread `drain_to_console()` → `godot_print!`. **Never call `godot_print!` from a worker thread.**

Both queues drain via `Callable::from_fn` on `SceneTree::process_frame` (NOT from `EditorPlugin::_process()` — that holds `bind_mut` which deadlocks on re-entrant `EditorInterface` calls).

## gdext API constraints

- `Dictionary::get(key)` returns `Option<Variant>` (single arg, no default). Unwrap with `.map(|v| v.to::<T>()).unwrap_or(default)`.
- `ProjectSettings::get_setting(name)` takes `impl AsArg<GString>` — pass `&str` or `GString`, not `&StringName`.
- Prefer `ResourceSaver::singleton().save_ex(resource).path(path).done()` for saving, or `resource.set_path()` first.
- Use `godot::tools::try_load::<T>(path)` (returns `Result`) over `ResourceLoader::singleton().load() + cast`.
- `resolve_node(&root, path)` in `commands/mod.rs` accepts `""`, `"."`, `"/"`, `"/root"`, root name, or `"RootName/Child"` (prefix auto-stripped). Use it for all node lookups.
- JSON↔Variant: `j2v`/`v2j` handle `Vector2/3/4`, `Color`, `Rect2`, `Quaternion`, `Resource`. Always use them.
- Scene-file ops must use `EditorInterface` — the editor won't see direct `.tscn` file writes.
- After writing files, call `EditorInterface::singleton().get_resource_filesystem().update_file()` so the editor detects changes.
- For subdirectories: `DirAccess::open("res://")` → `make_dir_recursive()`.

## Editor-mode limitations

- `get_variable`/`set_variable`: `@export` only in editor. Non-exported uses `PlaceHolderScriptInstance`.
- `validate_gdscript`: requires Editor Settings → Network → Language Server → Enable = ON. Creates a temporary tokio runtime.
- `add_circle_collision`/`add_rectangle_collision`: detects existing `CollisionShape2D`; `mode` field indicates action path.
- `rename_scene`: errors if target open but not active tab.
- `find_nodes_by_name`: case-sensitive substring (`contains`), not glob.
- `get/set_node_collision_layer/mask`: errors for non-CollisionObject2D/3D.
- `set_node_texture`: loads as `Texture2D` (not generic `Resource`). Default property `"texture"`.
- `set_project_setting`/`set_main_scene`: calls `ProjectSettings::save()`; `warning` field on failure.

## Build system (CMake + Corrosion)

```
py -3 build.py                        # debug + addons.zip
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # cargo clean + wipe addons/bin/
py -3 build.py --no-zip               # skip zip (fast iteration)
py -3 build.py --no-server            # only rebuild dll
```

**CI gates** (`.github/workflows/ci.yml`, Ubuntu): `cargo fmt --check --all` → `cargo clippy --workspace -- -D warnings` → `cmake -B build -S .` → `cmake --build build --config Debug` → `cargo test --workspace` (50 tests, offline, no Godot).

**On Windows**: `python`/`python3` are Microsoft Store stubs — use `py -3`.

**Gotchas**:
- Restart MCP client after rebuilding — it holds the old `godot-mcp-server.exe`. CMake auto-kills; if manual, `taskkill`/`pkill` first.
- `godot_mcp_gdext.dll` locked while Godot editor has plugin loaded. Close editor or disable plugin before rebuild.
- Pinned deps: `godot = "=0.5"`, `rmcp = "=1.7"`. Don't bump without testing.
- `Cargo.lock` is committed (binary crate). Don't regenerate casually.
- `rust-toolchain.toml` pins channel `1.83.0`.
- `plugin.cfg` version auto-syncs from `Cargo.toml [workspace.package].version`. Bump cargo version, not plugin.cfg.
- `godot/addons/godot_mcp/bin/*` is `.gitignore`d (except `.gitkeep`).

## C# solution generation

`csharp_create_solution` generates `.sln` + `.csproj` in Rust directly — **no second Godot process** (avoids port 9500 conflict).

- SDK version from `Engine::get_version_info()` → `Godot.NET.Sdk/X.Y.Z[-pre.N]` (SemVer 2.0: `"beta2"` → `"beta.2"`)
- Project name from `dotnet/project/assembly_name` → `application/config/name` → project folder name
- `.csproj`: UTF-8 **without** BOM; `.sln`: UTF-8 **with** BOM
- `csharp_build` spawns `dotnet build` — can't run while editor holds assembly file lock

## Editor control tools (server-side)

`godot_editor_open/close/restart` handled in `server/handler.rs` — never reach gdext. Closing the editor kills WebSocket, so the response comes from the server process.

- `GODOT_PATH` env var required. Must be in MCP client `env` config (stdio servers don't inherit shell env).
- `project_path` defaults to `godot/` (test project). Relative paths resolve from CWD.
- Close: `taskkill /F /IM` (Windows) or `pkill -f` (Unix).
- Restart waits 500ms after kill.

## Key patterns

- All `cmd_*` are **free functions** — closures can't capture `&self` across `dispatcher.submit()`. Return `json!({"error": "..."})` on error; `pipe()` converts to `Result<Value, String>`.
- `FileAccess::file_exists()` checks files; `DirAccess::dir_exists_absolute()` checks directories.

## Reference

Full wiki: `.repo_wiki/en/index.md` (also available in Chinese at `.repo_wiki/zh/`).

- **README.md** says "35 tools" — actual count is **99**. Ignore that number.
