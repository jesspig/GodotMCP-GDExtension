# GodotMCP — Agent Instructions

MCP server that exposes the Godot 4.6+ editor to AI tools via 52 commands (49 gdext + 3 server-side editor control).

## Architecture (two-process, three-crate)

```
AI client ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                       (crates/server, bin)                         (crates/gdext, cdylib in Godot Editor)
                                  │                                          │
                                  └──── crates/core ── shared protocol types ┘
```

- **stdio is the only MCP transport in use**. `transport-streamable-http-server` is in deps but unwired.
- **IPC wire format**: JSON-RPC-like, types in `crates/core/src/protocol.rs`. Every tool call is `method = "tool_call"` with `{tool, args}` params; the gdext side routes via `ws_server::route_tool_call`.
- **Adding a tool requires both sides**: register schema in `crates/server/src/tool_registry.rs::register_defaults` AND add a routing arm in the matching handler module under `crates/gdext/src/commands/`. Exception: server-side tools (editor control) only need the registry + `handler.rs` match arm. Mismatch shows as "Unknown tool" or "handler not yet implemented".
- **Hardcoded counts in tests**: `tool_registry.rs` and `handler.rs` tests assert `total == 52`. Update them when adding/removing tools.

## Handler routing chain (7 groups, 52 tools)

```
server handler → EditorControl (3, server-side) ────────────────────────────────────────┐
gdext route_tool_call → MetaCommands (4) → NodeCommands (16) → SceneCommands (15)      │
                     → ScriptGdCommands (6) → ScriptCsCommands (6) → SearchCommands (2) │
                                                                                         │
Editor control tools are intercepted in handler.rs BEFORE forward_tool_call() ←──────────┘
```

| Handler | File | Tools |
|---------|------|-------|
| EditorControl | `server/handler.rs` | godot_editor_open, godot_editor_close, godot_editor_restart |
| MetaCommands | `commands/meta.rs` | ping, get_engine_version, get_plugin_version, get_server_version |
| NodeCommands | `commands/node.rs` | 16 node CRUD/property/script/find tools |
| SceneCommands | `commands/scene.rs` | 15 scene file/editor tab tools |
| ScriptGdCommands | `commands/script_gd.rs` | 6 GDScript tools (create/read/edit/validate/list/eval) |
| ScriptCsCommands | `commands/script_cs.rs` | 6 C# tools (create/read/edit/list/build/create_solution) |
| SearchCommands | `commands/search.rs` | find_in_file, search_project |

## The main-thread / tokio split (most important gotcha)

Tool routing runs on tokio worker threads. **Almost every Godot API panics if called off the main thread**. Two mechanisms handle this:

1. **`dispatcher::MainThreadDispatcher`** (`dispatcher.rs`): worker submits a closure via `submit()`, returns a oneshot future. Main thread drains the queue. **All `cmd_*` functions are invoked through this.**
2. **`logging` module** (`logging.rs`): worker calls `log_info/log_warn/log_error` → `mpsc` channel + `eprintln!`. Main thread calls `drain_to_console()` to flush to `godot_print!`. **Never call `godot_print!` from a worker thread.**

Both queues drain via `Callable::from_fn` on `SceneTree::process_frame` (NOT from plugin `process()` — that holds `bind_mut` which deadlocks on re-entrant `EditorInterface` calls).

**New tool handlers must dispatch through `dispatcher.submit()`.** Do not call Godot APIs directly from `route_tool_call`.

## gdext API constraints

- **Do NOT read gdext source under `~/.cargo/registry/`**. Use Context7 library `/websites/godot-rust_github_io_gdext_master` or Godot docs at `https://docs.godotengine.org/en/4.6/classes/...`.
- `Dictionary::get(key)` returns `Option<Variant>` (single arg, no default). Unwrap with `.map(|v| v.to::<T>()).unwrap_or(default)`.
- `ProjectSettings::get_setting(name)` takes `impl AsArg<GString>` — pass a `&str` or `GString`, not `&StringName`.
- `ResourceSaver::save(&mut self, resource)` is a method on a `&mut ResourceSaver` singleton, takes only the resource (path is set on the resource). Prefer `ResourceSaver::singleton().save_ex(resource).path(path).done()` for saving to a specific path, or set `resource.set_path()` first.
- Use `godot::tools::try_load::<T>(path)` (returns `Result`) rather than `ResourceLoader::singleton().load() + cast`.
- `resolve_node(&root, path)` in `commands/mod.rs` accepts `""`, `"."`, `"/"`, `"/root"`, root name, or any `NodePath`. Use it for all node lookups.
- JSON ↔ Variant conversion: `j2v` / `v2j` in `commands/mod.rs` handle `Vector2/3/4`, `Color`, `Rect2`, `Quaternion`, `Resource`. Always use them; bypassing writes garbage.
- Scene-file ops must use `EditorInterface` methods — the editor won't see direct `.tscn` file writes.

## Commands

```
py -3 package_addons.py          # kills server.exe, builds both crates (debug),
                                      # copies dll to godot/addons/godot_mcp/bin/, zips addons.zip
py -3 package_addons.py --release       # release profile
py -3 package_addons.py --clean         # cargo clean + wipe addons/bin/ first
py -3 package_addons.py --no-zip        # skip addons.zip (fast iteration)
py -3 package_addons.py --no-server     # only rebuild the dll

cargo fmt --check --all                 # CI gate 1
cargo clippy --workspace -- -D warnings # CI gate 2
cargo build --workspace                 # CI gate 3
cargo test --workspace                  # CI gate 4 (50 tests, all offline / no Godot needed)
```

CI (`.github/workflows/ci.yml`) runs those four steps on Ubuntu. **On Windows, `python`/`python3` are Microsoft Store stubs that hang silently — use `py -3`.**

## Build / packaging gotchas

- **Restart MCP client after rebuilding** — it keeps the old `godot-mcp-server.exe` loaded. `package_addons.py` runs `taskkill /F /IM godot-mcp-server.exe` first; if building manually, kill it yourself.
- `godot_mcp_gdext.dll` is locked while the Godot editor has the plugin loaded. Close editor or disable plugin before rebuilding.
- Dependencies pinned: `godot = "=0.5"`, `rmcp = "=1.7"`. Don't bump without testing — gdext API changes frequently.
- `Cargo.lock` IS committed (binary crate). Do not regenerate casually.
- `plugin.cfg` has `script=""` — intentional, all logic is native.
- `godot/addons/godot_mcp/bin/*` is `.gitignore`d except `.gitkeep`. Never check in built artifacts.
- `plugin.cfg` version auto-syncs from `Cargo.toml [workspace.package].version` in `package_addons.py`. Bump the cargo version, not plugin.cfg.

## C# solution generation

`csharp_create_solution` generates `.sln` + `.csproj` directly in Rust — **does NOT spawn a second Godot process** (that caused WebSocket port 9500 conflicts).

- SDK version read from `Engine::get_version_info()` → `Godot.NET.Sdk/X.Y.Z[-pre.N]` (SemVer 2.0: `"beta2"` → `"beta.2"`)
- Project name from `dotnet/project/assembly_name` setting → `application/config/name` → project folder name
- `.csproj`: UTF-8 **without** BOM; `.sln`: UTF-8 **with** BOM
- Three sln configs: `Debug|Any CPU`, `ExportDebug|Any CPU`, `ExportRelease|Any CPU`
- Optional `enable_nativeaot` param adds `<PublishAot>true</PublishAot>` + `<TrimmerRootAssembly>` items
- `csharp_build` spawns `dotnet build` — cannot run while editor holds the assembly file lock

## Editor control tools (server-side)

`godot_editor_open`, `godot_editor_close`, `godot_editor_restart` are handled in `server/handler.rs` — they never reach the gdext side. This is intentional: closing the editor kills the WebSocket connection, so the response must come from the server process itself.

- `GODOT_PATH` env var specifies the Godot executable. Must be set in MCP client `env` config (stdio servers don't inherit shell env).
- `project_path` defaults to `godot/` (the test project). Pass an absolute path or relative to CWD.
- Editor close uses `taskkill /F /IM` (Windows) or `pkill -f` (Unix).
- Restart waits 500ms after kill before re-spawning.

## Key command implementation patterns

- All `cmd_*` are **free functions** (not methods) — closures cannot capture `&self` across `dispatcher.submit()`.
- Return `json!({"error": "..."})` for errors; the `pipe()` helper converts to `Result<Value, String>`.
- After writing files, call `EditorInterface::singleton().get_resource_filesystem().update_file()` so the editor sees changes.
- For subdirectory creation, use `DirAccess::open("res://")` → `make_dir_recursive()` before writing.
- `FileAccess::file_exists()` checks file existence; `DirAccess::dir_exists_absolute()` checks directories.

## Reference docs

The repository wiki (`.repo_wiki/`) is the long-form companion. Start at [`.repo_wiki/index.md`](.repo_wiki/index.md).

| Need | Page |
|------|------|
| Mental model of the two processes, three crates | [`.repo_wiki/overview/architecture.md`](.repo_wiki/overview/architecture.md) |
| **Read before touching `crates/gdext/`** — threading, pump, `bind_mut` trap | [`.repo_wiki/overview/threading-model.md`](.repo_wiki/overview/threading-model.md) |
| File-by-file map per crate | [`.repo_wiki/crates/{core,server,gdext}.md`](.repo_wiki/crates/) |
| Adding a new tool (server + gdext checklist) | [`.repo_wiki/modules/command-routing.md`](.repo_wiki/modules/command-routing.md) |
| JSON↔Variant rules, `resolve_node`, `try_load` | [`.repo_wiki/modules/scene-commands.md`](.repo_wiki/modules/scene-commands.md) |
| Catalog of all tools with args/returns | [`.repo_wiki/reference/tools-catalog.md`](.repo_wiki/reference/tools-catalog.md) |
| `package_addons.py` flags and file-lock recovery | [`.repo_wiki/reference/build-and-package.md`](.repo_wiki/reference/build-and-package.md) |
| Per-client MCP config quirks | [`.repo_wiki/reference/client-quirks.md`](.repo_wiki/reference/client-quirks.md) |
| Wire format details | [`.repo_wiki/specification/ipc-protocol.md`](.repo_wiki/specification/ipc-protocol.md) |
| Design decisions | [`.repo_wiki/design/decisions.md`](.repo_wiki/design/decisions.md) |
| Append-only changelog | [`.repo_wiki/log.md`](.repo_wiki/log.md) |

When you touch a pattern documented in the wiki, update the matching page and add an entry to `.repo_wiki/log.md`.

The roadmap lives under [`docs/plan/`](docs/plan/index.md). The wiki describes what *is*; `docs/plan/` describes what's *next*. When a planned item ships, move its description into the matching wiki page and delete the plan entry.
