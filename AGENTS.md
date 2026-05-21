# GodotMCP — Agent Instructions

MCP server that exposes the Godot 4.6+ editor to AI tools via 35 in-editor commands.

## Architecture (two-process, three-crate)

```
AI client ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                       (crates/server, bin)                         (crates/gdext, cdylib in Godot Editor)
                                  │                                          │
                                  └──── crates/core ── shared protocol types ┘
```

- **stdio is the only MCP transport in use**. `transport-streamable-http-server` is in deps but unwired (no `transports/` module).
- **IPC wire format**: JSON-RPC-like, types in `crates/core/src/protocol.rs`. Every tool call from the server is wrapped as `method = "tool_call"` with `{tool, args}` params; the gdext side routes via `ws_server::route_tool_call` → `MetaCommands` (4 tools) or `SceneCommands` (31 tools). Adding a tool requires **both** registering schema in `crates/server/src/tool_registry.rs::register_defaults` AND adding a routing arm in `crates/gdext/src/commands/scene.rs::handle_scene_tool` (or a new handler module). The server `handle_tool_call` forwards anything `registry.has_tool()` knows; mismatch between the two sides shows as "Unknown tool" or "handler not yet implemented".
- **Hardcoded counts in tests**: `tool_registry.rs` and `handler.rs` tests assert exact tool counts (currently 35). Update them when adding/removing tools or the test suite breaks.

## The main-thread / tokio split (most important gotcha)

Tool routing runs on tokio worker threads. **Almost every Godot API panics if called off the main thread** with `attempted to access binding from different thread than main thread`. Two mechanisms handle this:

1. **`dispatcher::MainThreadDispatcher`** (`crates/gdext/src/dispatcher.rs`): worker submits a closure via `submit()`, returns a oneshot future. Main thread drains the queue and runs closures. **All `cmd_*` functions in `scene.rs` are invoked through this.**
2. **`logging` module** (`crates/gdext/src/logging.rs`): worker calls `log_info/log_warn/log_error`, which pushes a `LogRecord` over an `mpsc` channel AND mirrors to `eprintln!`. Main thread calls `drain_to_console()` to flush queued records to `godot_print!/godot_warn!/godot_error!`. **Never call `godot_print!` from a worker thread.**

Both queues are drained by a **`Callable::from_fn` connected to `SceneTree::process_frame`** (set up in `editor_plugin::install_main_thread_pump`). They are **deliberately NOT** drained from the plugin's `process(&mut self)` callback. Reason: `process` holds an active `bind_mut` on `McpEditorPlugin`; any closure executing `EditorInterface` calls that synchronously re-enter the plugin (via signals) triggers `Gd<T>::bind_mut() failed, already bound` (gdext issue #338). The `process_frame` pump runs in a call stack with no plugin borrow held.

If you add new tool handlers, **dispatch them through `dispatcher.submit()`**. Do not call Godot APIs directly from `route_tool_call`.

## Scene command conventions (`crates/gdext/src/commands/scene.rs`)

- All `cmd_*` are free functions, not methods — closures cannot capture `&self` across `dispatcher.submit()`.
- `resolve_node(&root, path)` is the standard way to look up a node. It accepts `""`, `"."`, `"/"`, `"/root"`, the root's name, or any normal `NodePath`. Use it instead of `root.get_node_or_null` directly so tools work when targeting the scene root.
- JSON ↔ Variant conversion is in `j2v` / `v2j`. They handle `Vector2`/`Vector3`/`Vector4`/`Color`/`Rect2`/`Quaternion`/`Resource` via key-shape detection (`{x,y}`, `{r,g,b,a}`, `{position,size}`, `resource_path`, `res://...` string). Bypassing them and stuffing a JSON object into `node.set()` writes garbage (Godot falls back to defaults like `(1e-05, 1e-05)`).
- Use `godot::tools::try_load::<T>(path)` (returns `Result<Gd<T>, IoError>`) rather than `ResourceLoader::singleton().load() + cast`. Better errors, type-correct.
- Scene-file ops use `EditorInterface` (e.g. `open_scene_from_path_ex`, `save_scene`, `get_open_scenes`). Don't write to `.tscn` files directly — the editor won't see the changes.

## Commands

```
py -3 package_addons.py          # default: kills server.exe, builds both crates (debug),
                                 # copies dll to addons/godot_mcp/bin/, zips addons.zip,
                                 # prints artifact summary with mtimes
py -3 package_addons.py --release       # release profile
py -3 package_addons.py --clean         # cargo clean + wipe addons/bin/ first
py -3 package_addons.py --no-zip        # skip addons.zip (fast iteration)
py -3 package_addons.py --no-server     # only rebuild the dll

cargo fmt --check --all                 # CI gate 1
cargo clippy --workspace -- -D warnings # CI gate 2
cargo build --workspace                 # CI gate 3
cargo test --workspace                  # CI gate 4 (47 tests, all offline / no Godot needed)
```

CI (`.github/workflows/ci.yml`) runs exactly those four steps in order on Ubuntu. Reproduce locally before pushing — clippy is `-D warnings` and will reject anything new.

**On Windows the `python` / `python3` commands are Microsoft Store stubs and hang silently. Use `py -3`.**

## Build / packaging gotchas

- The MCP client launches `target/debug/godot-mcp-server.exe` (or release) directly via the path in the client's `mcpServers` config. **You must restart the MCP client after rebuilding** or it keeps the old binary loaded.
- Cargo will fail to overwrite `godot-mcp-server.exe` if a client still has it loaded → `package_addons.py` runs `taskkill /F /IM godot-mcp-server.exe` first. If you run `cargo build` manually and get a file-lock error, kill the process yourself.
- `target/debug/godot_mcp_gdext.dll` is locked while the Godot editor has the plugin loaded. Either disable the plugin first or close the editor before rebuilding.
- Cargo dependencies are pinned: `godot = "=0.5"`, `rmcp = "=1.7"`. Don't bump without testing — gdext API changes frequently and rmcp 1.x has had breaking minor releases.
- `Cargo.lock` IS committed (binary crate). Do not regenerate casually.
- `rust-toolchain.toml` pins `stable` with `rustfmt`+`clippy`.
- `plugin.cfg` has `script=""` — intentional. All logic is native.
- Addon binaries: `addons/godot_mcp/bin/*` is `.gitignore`d except `.gitkeep`. `package_addons.py` repopulates it; never check in built artifacts.
- `plugin.cfg` version auto-syncs from `Cargo.toml [workspace.package].version` in `package_addons.py`. Bump the cargo version, not plugin.cfg.

## Reference docs

The repository wiki (`.repo_wiki/`) is the long-form companion to this file. Start at [`.repo_wiki/index.md`](.repo_wiki/index.md) and follow the section that matches the task:

| Need | Page |
|------|------|
| Mental model of the two processes, three crates | [`.repo_wiki/overview/architecture.md`](.repo_wiki/overview/architecture.md) |
| **Read before touching `crates/gdext/`** — the tokio↔main-thread split, pump pattern, `bind_mut` trap | [`.repo_wiki/overview/threading-model.md`](.repo_wiki/overview/threading-model.md) |
| File-by-file map per crate | [`.repo_wiki/crates/{core,server,gdext}.md`](.repo_wiki/crates/) |
| Adding a new tool (server side + gdext side checklist) | [`.repo_wiki/modules/command-routing.md`](.repo_wiki/modules/command-routing.md) |
| JSON↔Variant rules, `resolve_node`, `try_load` | [`.repo_wiki/modules/scene-commands.md`](.repo_wiki/modules/scene-commands.md) |
| Catalog of all 35 tools with args/returns | [`.repo_wiki/reference/tools-catalog.md`](.repo_wiki/reference/tools-catalog.md) |
| `package_addons.py` flags and file-lock recovery | [`.repo_wiki/reference/build-and-package.md`](.repo_wiki/reference/build-and-package.md) |
| Per-client MCP config quirks (`servers` vs `mcpServers`, `serverUrl`, `httpUrl`, Codex TOML, …) | [`.repo_wiki/reference/client-quirks.md`](.repo_wiki/reference/client-quirks.md) |
| Wire format details (`IpcRequest`/`IpcResponse`/`ToolCallParams`) | [`.repo_wiki/specification/ipc-protocol.md`](.repo_wiki/specification/ipc-protocol.md) |
| Why the code is shaped the way it is | [`.repo_wiki/design/decisions.md`](.repo_wiki/design/decisions.md) |
| Append-only project changelog | [`.repo_wiki/log.md`](.repo_wiki/log.md) |

When you touch a pattern documented in the wiki, update the matching page in the same change and add an entry to `.repo_wiki/log.md`.

The forward-looking roadmap (planned phases, open ADR questions) lives under [`docs/plan/`](docs/plan/index.md). The wiki describes what *is*; `docs/plan/` describes what's *next*. When a planned item ships, move its description from `docs/plan/` into the matching wiki page and delete the plan entry.

For Godot / gdext API questions, query the Context7 library `/websites/godot-rust_github_io_gdext_master` or fetch `https://docs.godotengine.org/en/4.6/classes/...`. **Do NOT read the gdext source under `~/.cargo/registry/`** — the user has rejected that approach.
