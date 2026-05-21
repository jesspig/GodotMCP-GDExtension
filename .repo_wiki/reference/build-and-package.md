# Build and Package

> Exact commands you'll likely guess wrong, plus the file-lock and PATH gotchas that come up on Windows.

## One-shot rebuild

```bash
py -3 package_addons.py
```

Defaults:

1. Reads version from `[workspace.package].version` in `Cargo.toml` and updates `addons/godot_mcp/plugin.cfg` to match.
2. `taskkill /F /IM godot-mcp-server.exe /T` (or `pkill -f godot-mcp-server` on POSIX) so cargo can overwrite the binary.
3. `cargo build -p godot-mcp-gdext` (debug).
4. `cargo build -p godot-mcp-server` (debug).
5. Copies `target/debug/godot_mcp_gdext.{dll,so,dylib}` to `addons/godot_mcp/bin/`.
6. Zips `addons/` into `addons.zip`.
7. Prints a `BUILD SUMMARY` with the path, size, and mtime of every produced artefact — that's how you tell whether the rebuild actually took effect.

## Flags

| Flag | Effect |
|------|--------|
| `--release` | Use `--release` profile; copy from `target/release/...`. |
| `--clean` | `cargo clean` (~5 GB) + wipe `addons/godot_mcp/bin/`. |
| `--no-zip` | Skip the `addons.zip` step (fastest iteration). |
| `--no-server` | Skip `godot-mcp-server` (only rebuild the dll). |

Combine freely (`py -3 package_addons.py --release --no-zip`).

## CI gates — run in this order before pushing

```bash
cargo fmt --check --all                       # 1
cargo clippy --workspace -- -D warnings       # 2
cargo build --workspace                       # 3
cargo test --workspace                        # 4   (47 tests, all offline)
```

`.github/workflows/ci.yml` runs exactly those four steps on Ubuntu. Clippy is `-D warnings` — any new lint will fail the PR. Tests are Godot-free; they assert hard-coded tool counts (currently 35) and protocol round-trips.

## Python launcher trap (Windows)

The `python`, `python3`, and `py.exe` commands in `C:\Users\<user>\AppData\Local\Microsoft\WindowsApps\` are **App Execute Aliases / Windows Store stubs**. Calling `python` or `python3` from a non-interactive shell can hang forever waiting on a UI prompt. Always use `py -3` instead — it dispatches to the actual installed Python.

## File-lock symptoms during cargo build

| Symptom | Cause | Fix |
|---------|-------|-----|
| `error: failed to remove file 'target/debug/godot-mcp-server.exe'` `os error 5` | MCP client (Claude Code / OpenCode / etc.) still has the binary loaded over stdio | `package_addons.py` already taskkills it; if you ran `cargo build` directly, run `taskkill /F /IM godot-mcp-server.exe` and retry. **Restart the MCP client after rebuild** or it just respawns the old PID. |
| `error: failed to remove file 'target/debug/godot_mcp_gdext.dll'` | Godot editor has the plugin loaded | Disable the plugin via *Project → Project Settings → Plugins*, or close the editor before rebuilding. `reloadable = true` is in the `.gdextension` manifest but cargo still can't overwrite a file with an open handle on Windows. |

## What gets shipped vs what gets committed

- **Committed**: source, `Cargo.toml`, `Cargo.lock` (binary crate — do not regenerate casually), `addons/godot_mcp/{plugin.cfg, godot_mcp.gdextension, bin/.gitkeep}`, `rust-toolchain.toml`, CI workflows.
- **Gitignored**: `target/`, `addons/godot_mcp/bin/*` (except `.gitkeep`), `addons.zip`, every `*.dll`/`*.so`/`*.dylib` in the repo. `package_addons.py` produces all of these on demand.

The `addons.zip` is the distribution artefact — extract into a Godot project's root and the plugin appears under `addons/godot_mcp/`. The MCP server binary is a separate path the user puts in their MCP client config.

## Version bumping

Bump the workspace version in `Cargo.toml`. `package_addons.py` syncs it into `plugin.cfg`. Do not edit `plugin.cfg`'s version directly.

```toml
# Cargo.toml
[workspace.package]
version = "0.1.0"
edition = "2024"
```

## Hot reload during development

`addons/godot_mcp/godot_mcp.gdextension` has `reloadable = true`, so the Godot editor will pick up a freshly copied dll on extension reload **as long as nothing else holds an open handle**. In practice:

- Save & rebuild → close the editor → run `py -3 package_addons.py --no-zip --no-server` → restart the editor. (Fastest loop when only `cmd_*` changed.)
- For server-side changes: kill the MCP client first (else cargo can't overwrite the .exe). Restart the client after; some clients re-spawn on next tool call, others need a process kill.

## Cargo dep pins worth knowing

| Crate | Pin | Reason |
|-------|-----|--------|
| `godot` | `=0.5` | gdext API breaks between minor versions; `0.5.x` is verified. |
| `rmcp` | `=1.7` | rmcp 1.x has had breaking minor releases. |

Anything else (`tokio`, `serde`, `axum`, `clap`) is loose-pinned to the major version. Renovate without ceremony.
