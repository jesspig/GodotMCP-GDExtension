# Design Decisions

> ADR-style records of key architectural decisions.

## ADR-001: Dual-Process Architecture (Server + GDExtension)

**Status**: Accepted  
**Date**: Initial project setup  
**Context**: GDExtension CDyLib can load into Godot editor and access native APIs, but can't act as a standalone MCP server. Meanwhile, an MCP server (a standard Rust binary) can handle stdio, rmcp, and AI client interactions, but can't call Godot APIs.  
**Decision**: Split the system into two processes connected via WebSocket.  
**Consequences**:
- Positive: Clean separation of concerns; each component can be developed and tested independently
- Positive: MCP server can start/restart independently from editor lifecycle
- Negative: WebSocket IPC adds latency; introduces protocol version sync concerns
- Negative: Requires handling startup order and connection management between two processes

## ADR-002: WebSocket over Unix Domain Socket / Named Pipe

**Status**: Accepted  
**Context**: Need a cross-platform IPC solution. Unix domain sockets don't work well on Windows (Named Pipes are more appropriate but awkward in Rust).  
**Decision**: Use WebSocket (ws://127.0.0.1:9500), which is cross-platform and well-supported in the Rust ecosystem.  
**Consequences**:
- Positive: `tokio-tungstenite` works out of the box on Windows, macOS, Linux
- Positive: Easy to debug with tools like wscat
- Negative: Port conflict risk if users open multiple Godot instances

## ADR-003: `process_frame` over `EditorPlugin::_process()` for Dispatcher Pump

**Status**: Accepted  
**Context**: `EditorPlugin` trait provides `_process(&mut self, delta)`, but calling certain Godot APIs (like `EditorInterface`) in that context triggers `Gd::bind_mut()` deadlocks.  
**Decision**: Use `Callable::from_fn` connected to `SceneTree::process_frame` to pump the dispatcher and log queues, rather than overriding `_process()`.  
**Consequences**:
- Positive: Completely avoids `bind_mut` deadlocks
- Positive: `McpEditorPlugin::_process()` is intentionally empty — clearly communicates not to put logic there
- Negative: Adds a layer of indirection

## ADR-004: Static Tool Registration, No Runtime Query from GDExt

**Status**: Accepted  
**Context**: MCP protocol requires the server to respond to `list_request_tools` with tool list and full JSON Schemas.  
**Decision**: Declare all 99 tool schemas statically on the server side. GDExt side has a second registry but only for tool name discovery and routing.  
**Consequences**:
- Positive: `list_request_tools` can respond without Godot editor running (server can start independently)
- Positive: Schemas live close to their consumer (server)
- Negative: Dual registration (server + gdext) must stay in sync — both sides must be updated when adding/removing tools
- Negative: Tests depend on hardcoded `total == 99` assertions

## ADR-005: CMake + Corrosion Build (Not Standalone Cargo)

**Status**: Accepted  
**Context**: Project needs to build a Rust CDyLib (`godot_mcp_gdext.dll`) and a Rust binary (`godot-mcp-server.exe`). Post-build steps involve copying DLL, generating `plugin.cfg`, packaging `addons.zip`, etc.  
**Decision**: Use CMake as the top-level build system with Corrosion for Rust integration.  
**Consequences**:
- Positive: Unified build entry point — `cmake --build`
- Positive: CMake handles cross-platform tasks (process termination, file ops, CPack packaging)
- Negative: Requires CMake + Corrosion — not a pure Cargo workflow
- Negative: `build.py` wrapper is a convenience, not strictly required

## ADR-006: C# Solution Direct Generation (No Second Godot Process)

**Status**: Accepted  
**Context**: Godot's `--generate-mono-solution` flag spawns a new editor process, competing with our gdext plugin for WebSocket port 9500.  
**Decision**: Generate `.sln` and `.csproj` files directly in Rust, without spawning an editor process.  
**Consequences**:
- Positive: No port conflicts
- Positive: Faster (no process overhead)
- Negative: Must maintain templates compatible with Godot 4.6 .NET SDK format
- Negative: Solution generation logic must track Godot .NET SDK version changes

## ADR-007: Cross-Thread Logging via mpsc + eprintln Mirror

**Status**: Accepted  
**Context**: Godot's `godot_print!` macro has undefined behavior (typically crashes) when called from a non-main thread.  
**Decision**: Tokio worker threads call `log_info/log_warn/log_error` → messages enter mpsc channel + `eprintln!` to terminal. Main thread forwards to `godot_print!` from the `process_frame` signal pump.  
**Consequences**:
- Positive: Log calls from worker threads are reliable and immediate
- Positive: Logs eventually appear in both Godot console and client terminal
- Negative: Godot console logs may be delayed by up to one frame

## ADR-008: `call_method` uses `Object::call()` (not Deferred/Callable)

**Status**: Accepted  
**Context**: Need a generic way to call any method on a node.  
**Decision**: Use `node.call(method, &args)` directly.  
**Consequences**:
- Positive: Supports arbitrary method signatures
- Positive: Immediate execution (gdext already running on main thread)
- Negative: May fail at runtime if method expects different types than passed
