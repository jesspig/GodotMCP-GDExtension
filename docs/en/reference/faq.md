# FAQ

## Connection Issues

### Cannot connect after GodotMCP starts

1. Verify the plugin is enabled in Project Settings
2. Check that the port is not occupied: `curl http://localhost:9600/mcp`
3. Check if `GODOT_MCP_HTTP_PORT` is set to a non-default port
4. Close and reopen the editor

### Some tools unavailable after successful connection

Tools are auto-collected via the X-macro registration mechanism. All `extensions/src/built_in/tools/**/*.hpp` files are included by `register_itools.cpp` via `#include`, requiring no `.cpp` files or codegen. If a tool is unavailable, ensure its header is correctly placed in the tools directory and registered in the corresponding X-macro registration file.

## Build Issues

### Windows build hangs

Use `py -3` instead of `python`. The Microsoft Store python shim may hang silently.

### MSB4019 / VCTargetsPath build error

`main.py` automatically detects the Visual Studio installation path and sets `VCTargetsPath`. If it fails, try:

```bash
uv run python main.py build --clean
```

### Plugin reload fails

The Godot editor locks the DLL when the plugin is loaded. Close the editor or disable the plugin in Project Settings before rebuilding.

## Editor Issues

### @export variables not registering

When attaching scripts, call in this order:

1. `EditorInterface::get_resource_filesystem()->update_file(path)`
2. `ResourceLoader::load(path)`
3. `GDScript::set_source_code(src)`
4. `GDScript::reload()`

Missing any step may cause `@export` variables to not display.

### Node persists after create_node undo

The `create_node` undo operation must call `add_do_reference(node)` in `EditorUndoRedoManager` to prevent Godot from garbage-collecting the newly created node before undo.

## Other

### Why C++ instead of GDScript?

GDExtension directly operates Godot's low-level engine API, providing better performance and more complete API access.

### How to change the listening port?

Set the environment variable:

```bash
# Windows
set GODOT_MCP_HTTP_PORT=9700

# Linux / macOS
export GODOT_MCP_HTTP_PORT=9700
```

### Submitting Issues or Contributing

GitHub repository: [https://github.com/jesspig/GodotMCP-GDExtension](https://github.com/jesspig/GodotMCP-GDExtension)
