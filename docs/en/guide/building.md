# Building from Source

## Prerequisites

- **Python 3.14+** (check `.python-version`)
- **uv** (Python package manager)
- **CMake 3.25+**
- **C++17 compiler** (MSVC, GCC, Clang, or AppleClang)
- **Git** (for FetchContent dependencies)

## Quick Build

```bash
uv run python main.py build
```

This performs a Debug build and copies the result to `example/addons/godot_mcp/bin/`.

## Build Options

### Release Build

```bash
uv run python main.py build --release
```

### Build with Packaging

```bash
uv run python main.py build --zip
```

Builds and then packages `addons.zip` for distribution.

### Parallel Jobs

```bash
uv run python main.py build -j 16
```

### Custom Compiler/Generator

```bash
uv run python main.py build --compiler clang-cl --generator ninja
```

### Compiler Cache

```bash
uv run python main.py build --compiler clang-cl --generator ninja --cache sccache
```

## Build Commands Reference

| Command | Description |
|---------|-------------|
| `uv run python main.py build` | Debug build |
| `uv run python main.py build --release` | Release build |
| `uv run python main.py build --zip` | Build + package addons.zip |
| `uv run python main.py build --clean` | Clean build cache (keep dependencies) |
| `uv run python main.py package` | Package prebuilt libs into addons.zip |
| `uv run python main.py test` | Full test pipeline |

## CI Recipes

### Windows (clang-cl + ninja + sccache)

```bash
uv run python main.py build --compiler clang-cl --generator ninja --cache sccache
```

### Linux (gcc + ninja + sccache)

```bash
uv run python main.py build --compiler gcc --generator ninja --cache sccache
```

### macOS (appleclang + ninja + sccache)

```bash
uv run python main.py build --compiler appleclang --generator ninja --cache sccache
```

## Important Notes

- **DLL lock**: Godot editor locks the DLL. Close the editor before rebuilding.
- **Dependencies**: `godot-cpp 10.0.0-rc1` and `ryml v0.7.0` are fetched via CMake FetchContent.
- **Version number**: Single source of truth in root `CMakeLists.txt` (`PROJECT_VERSION`).
- **Auto-generated configs**: `plugin.cfg` and `.gdextension` are regenerated every build.