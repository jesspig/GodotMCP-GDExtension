# Client Quirks

## General Notes

- **stdio is the only transport**. Make sure config uses `command` not `url`.
- All clients need `GODOT_PATH` env var set manually.

## Known Issues

| Issue | Scenario | Cause / Workaround |
|-------|----------|-------------------|
| `godot-mcp-server` starts but times out | All clients | Godot editor not restarted or bad `GODOT_PATH`. Check path and use `godot_editor_open` tool |
| Windows `python` stub issue | All Windows users | `python`/`python3` are Microsoft Store stubs — use `py -3` for scripts |
| Editor process locked | Rebuilding dll | Close Godot editor or disable plugin before rebuild. CMake auto-kills server process |
| C# Solution generation conflict | Multiple editor instances | WebSocket port 9500 already in use |

## Post-Build Steps

1. If developing the dll, **close Godot editor** (holds `godot_mcp_gdext.dll` file lock)
2. Run `py -3 build.py`
3. **Restart MCP client** (restart editor)
4. Use `godot_editor_open` tool to launch Godot

## Godot Editor Mode Limitations

| Tool | Limitation |
|------|------------|
| `get_variable` / `set_variable` | Only `@export` variables work in editor mode. Non-exported members use `PlaceHolderScriptInstance` |
| `validate_gdscript` | Requires Editor Settings → Network → Language Server → Enable = ON |
| `csharp_build` | Can't run simultaneously with editor (editor holds assembly file lock) |
| `rename_scene` | Returns error if target is open but not active tab |
| `add_circle_collision` / `add_rectangle_collision` | Detects existing `CollisionShape2D`; `mode` field indicates action taken |
