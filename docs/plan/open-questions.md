# Open Questions

> Items that block planning or design decisions until someone (probably the project owner) decides. Add to the top, resolve at the bottom, never silently delete.

## Active

### Q1 — HTTP bind address default

**Context**: Phase 4 introduces an HTTP listener. Today's WS server hardcodes `127.0.0.1:9500` because the assumption is single-developer-machine. Phase 4 plans `--http-bind 127.0.0.1` as the default, with `0.0.0.0` available as an opt-in.

**Question**: Are we OK keeping `0.0.0.0` as a CLI-only opt-in with no auth, or should we require an auth token whenever the bind is non-loopback?

**Why it matters**: a user who copies a tutorial blindly and types `--http-bind 0.0.0.0` exposes the editor to their LAN. Anyone on that LAN can `create_node`, `delete_scene`, `attach_script` (which can run arbitrary GDScript). Real damage potential.

**Options**:

- **A**: Document the risk loudly, ship without auth.
- **B**: Generate a random token on first run, require `Authorization: Bearer <token>` for non-loopback binds. Print the token to stderr and offer a dock action to regenerate.
- **C**: Refuse to bind anywhere but loopback. If someone really wants LAN access they can run an external proxy with their own auth.

Lean toward **C** until there's a concrete user request for LAN access.

### Q2 — Where does per-tool enabled state actually live?

**Context**: Today the server's `ToolRegistry` is the source of truth. The dock toggle pushes a `tool_list_updated` notification *from the editor to the server*. So:

- Toggle while no MCP server is connected → notification has no subscriber → toggle is lost.
- Toggle while the server is connected → state lives only in the server's in-memory registry.
- Server restart → state resets to "all enabled".

**Question**: Should per-tool state persist? Where?

**Options**:

- **A** (current behaviour, unchanged): per-session, in-memory in the server. Document it.
- **B**: Persist in `EditorSettings` on the gdext side; dock reads on startup and sends a `tool_list_updated` to the server on first connect. The editor becomes the source of truth.
- **C**: Persist on the server side in a JSON file next to the binary. The editor's dock reflects whatever the server reports.

**B** is the natural extension of the dock-driven model and matches user expectations ("I disabled `debug_*` once; it should stay disabled"). Worth doing alongside Phase 3 if Phase 3 also touches the dock.

### Q3 — Debug tool group: how deep?

**Context**: Phase 5's `debug_*` group is sketched as a thin layer over the Godot remote debugger. The protocol is poorly documented and the gdext exposure is partial.

**Question**: Do we want full remote-debugger parity (frame stepping, breakpoint set/clear, variable inspection inside the running game), or is "list remote nodes + read property" sufficient?

**Why it matters**: full parity is a significant engineering investment with low payoff if AI agents end up just stopping the game and inspecting the scene tree statically. Defer until there's a concrete agent workflow asking for live introspection.

**Default for now**: ship the minimum (list remote nodes, read remote property). Treat anything more as Phase 5b.

### Q4 — Should `package_addons.py` bundle the server binary into the addon?

**Context**: Phase 3.2 needs the dock to know where `godot-mcp-server` lives to write client configs. Three options were listed in the phase doc (file dialog, cargo metadata, bundling). Bundling is the cleanest but bloats `addons.zip` from ~1.7 MB to ~10 MB.

**Question**: Is the size acceptable for the convenience of "one zip, everything works"?

**Lean**: yes. 10 MB is fine for a developer tool; nobody will distribute this to end users.

## Resolved

_(nothing yet — move items here with a date and short rationale when they're closed)_
