# FAQ

## Connection Issues

### Q: AI client says "connection refused"

Verify Godot is running (editor, not just the project). Check the port:

```bash
curl http://127.0.0.1:9600/mcp -X POST -H "Content-Type: application/json" -d "{}"
```

If this fails, check:
- Godot is open with the plugin enabled
- Port 9600 is not in use by another application
- Firewall is not blocking the port

### Q: Port 9600 is already in use

Set a different port via `GODOT_MCP_HTTP_PORT` environment variable or ProjectSettings > `godot_mcp/http_port`. Update your AI client config to match.

## Build Issues

### Q: Windows build freezes or takes forever

Try using `clang-cl` with `ninja`:

```bash
uv run python main.py build --compiler clang-cl --generator ninja
```

### Q: SSL errors during CMake configure

The build system auto-retries with `CMAKE_TLS_VERIFY=0` on SSL errors. If it still fails, check your internet connection or proxy settings.

### Q: "DLL locked" error when building

Godot editor holds a lock on the plugin DLL. Close Godot before rebuilding.

## Runtime Issues

### Q: Tools return "scene required" error

Some tools need an open scene. Create or open a scene first before calling scene-dependent tools.

### Q: C# tools cause conflicts

The C# solution generation in GDExtension may conflict with Godot's own C# project management. See the API reference for details on proper C# tool registration.

### Q: set_node_property doesn't work for some values

The value must match the target property type. The fallback tool accepts all types, but the editor may reject type-incompatible values. Use `get_node_property` first to check the expected type.

## General

### Q: How do I update the plugin?

Download the latest `addons.zip` from Releases and extract it over your existing `addons/godot_mcp/` directory. Restart Godot.

### Q: Can I use this with C# Godot projects?

Yes. The plugin is a GDExtension (C++), which works alongside C# in Godot. The SDK also supports registering C# custom tools via `McpToolRegistry`.

### Q: Does this work in Godot 4.5 or earlier?

No. The plugin requires Godot 4.6+ (specifically the GDExtension API version 4.6).