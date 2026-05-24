# Build & Package

## Build System

The build system is **CMake + Corrosion** (Rust-CMake bridge). A thin `build.py` wrapper is provided.

```bash
py -3 build.py                        # debug build + addons.zip
py -3 build.py --release              # release build + addons.zip
py -3 build.py --clean                # cargo clean + wipe addons/bin/
py -3 build.py --no-zip               # skip addons.zip (fast iteration)
py -3 build.py --no-server            # only rebuild the dll
```

CMake handles:
- Killing running `godot-mcp-server.exe` (`taskkill`/`pkill`)
- Generating `plugin.cfg` and `godot_mcp.gdextension`
- Copying dll to `godot/addons/godot_mcp/bin/`
- CPack packaging → `addons.zip`

## Manual Build (skip build.py)

```bash
cmake -B build -S .                          # configure
cmake --build build --config Debug           # build
cmake --build build --config Debug --target package  # package
```

## CI Gates

`.github/workflows/ci.yml` (runs on Ubuntu):

```bash
cargo fmt --check --all                       # 1. Format check
cargo clippy --workspace -- -D warnings       # 2. Strict lint
cmake -B build -S .                           # 3a. CMake configure
cmake --build build --config Debug            # 3b. Build gdext + server
cargo test --workspace                        # 4. Tests (50, offline no Godot)
```

CI triggers on push and PR to `master` only.

## Release Build

`.github/workflows/release.yml` triggers on tag `v*`:

- Matrix: ubuntu / macOS / Windows
- Uses `cmake -DRELEASE=ON`
- Uploads GDExtension lib + Server binary per-platform to Release artifacts
- Assembles cross-platform `addons.zip` on Ubuntu (dll/so/dylib for 3 platforms)

### Windows Notes

- `python`/`python3` are Microsoft Store stubs that silently hang — **always use `py -3`**
- Release server binary is renamed to `godot-mcp-server_windows.exe`

## GDExtension File Locks

| File | Locked By | Handler |
|------|-----------|---------|
| `godot-mcp-server.exe` | Running MCP client | CMake `execute_process(taskkill)` auto-kills; manual: `taskkill /F /IM godot-mcp-server.exe` |
| `godot_mcp_gdext.dll` | Godot editor (plugin loaded) | Close editor or disable plugin before rebuild |

## Build Output Git Ignore

`godot/addons/godot_mcp/bin/*` is `.gitignore`d (except `.gitkeep`). **Never check in build artifacts.**

## Version Management

- Single version source: `Cargo.toml [workspace.package].version`
- CMakeLists.txt parses version from Cargo.toml, fills `plugin.cfg` automatically
- Bump cargo version; no need to manually edit `plugin.cfg`

## Pinned Dependencies

- `godot = "=0.5"` — strictly locked
- `rmcp = "=1.7"` — strictly locked
- `Cargo.lock` IS committed (binary crate); don't regenerate casually
