# Building & Packaging

## Build System

The build system is **CMake** (C++ GDExtension → godot-cpp 10.0.0-rc1 via FetchContent). A lightweight `main.py` wrapper is provided.

```bash
uv run python main.py build                 # debug build
uv run python main.py build --release       # release build
uv run python main.py build --zip           # build + addons.zip
uv run python main.py build --clean-cache   # clear build cache
uv run python main.py build --clean         # same as --clean-cache
uv run python main.py build -j 16           # parallel 16 jobs
uv run python main.py package               # package addons.zip
uv run python main.py test                  # run test pipeline
```

CMake handles automatically:

- `FetchContent` pulls `godot-cpp 10.0.0-rc1` + `ryml v0.7.0` (header-only)
- Generates `plugin.cfg` and `godot_mcp.gdextension`
- Copies dll/dylib/so to `example/addons/godot_mcp/bin/`
- CPack packaging → `addons.zip`
- Auto-detects sccache/ccache, Unity build, lld-link for faster compilation

## C++ GDExtension Build Flow

CMake builds godot-cpp + extension source files via `add_subdirectory(extensions)`:

1. FetchContent pulls `godot-cpp 10.0.0-rc1`
2. Adds all source files from `/extensions/src/`
3. Links godot-cpp static library → `godot_mcp_gdext.dll`
4. Post-process: copies to `example/addons/godot_mcp/bin/`

## Manual Build (without main.py)

```bash
cmake -B build -S .                          # Configure
cmake --build build --config Debug           # Build
cmake --build build --config Debug --target package  # Package
```

## Git Ignore

`example/addons/godot_mcp/bin/*` is in `.gitignore`. **Never commit build artifacts.**

## Version Management

- Single version source in `CMakeLists.txt`: `set(PROJECT_VERSION "0.2.1")`
- CMake auto-fills this version when generating `plugin.cfg`
- Update the CMake version; no need to manually edit `plugin.cfg`

## Dependency Locking

- `godot-cpp 10.0.0-rc1`: pinned tag via FetchContent
- `ryml v0.7.0`: header-only YAML library
- Python dependencies: locked via `uv.lock`
- **Always use `uv run python`** to run main.py (uv auto-activates `.venv`)
