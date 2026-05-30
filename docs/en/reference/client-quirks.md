# Client Notes

## General

- The Godot editor must be running with the MCP plugin loaded before clients can connect
- MCP Streamable HTTP is the only transport protocol; configure `url` as `http://localhost:9600/mcp`

### Known Issues

| Issue | Cause / Workaround |
|-------|--------------------|
| Plugin loaded but connection fails | Godot editor not running or port occupied. Check `GODOT_MCP_HTTP_PORT` and ensure port 9600 is free |
| `python` hangs on Windows | Microsoft Store stub issue — use `py -3` |
| DLL locked | Close the Godot editor or disable the plugin before rebuilding |
| C# Solution conflict | When opening multiple editor instances, HTTP port 9600 is taken by the first instance |

## Godot Editor Mode Limitations

| Tool | Limitation |
|------|------------|
| `get_variable` / `set_variable` | Only `@export` variables are available in editor mode |
| `validate_gdscript` | Requires Editor Settings → Network → Language Server → Enable to be ON |
| `csharp_build` | Cannot run concurrently with the editor (editor holds assembly file lock) |
| `rename_scene` | Returns error if the target is open but not the active tab |
