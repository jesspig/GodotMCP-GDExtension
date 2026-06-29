# Overview

GodotMCP is a **C++ GDExtension** plugin for Godot 4.6+ that exposes the editor to AI coding assistants through the **Model Context Protocol (MCP)**. It runs entirely inside the Godot editor process on the main thread — no separate server process, no threading, no locks.

## What It Does

GodotMCP acts as a bridge between AI tools (Claude Code, Cline, Continue, Cursor, opencode, Roo Code, etc.) and the Godot editor. AI assistants can:

- **Create and manage scenes** — add, delete, rename, duplicate, reparent nodes
- **Edit scripts** — read, write, patch GDScript and C# files
- **Manage files** — create, delete, move, copy files in the project
- **Control the editor** — run/stop projects, set breakpoints, capture viewports
- **Query the runtime** — inspect game scenes, get/set properties, call methods
- **Query Godot documentation** — search classes, methods, properties via ClassDB
- **And 150+ more operations** across every editor domain

## Key Features

| Feature | Description |
|---------|-------------|
| **164 built-in tools** | Covers scene tree, scripts, filesystem, animation, shaders, input, physics, runtime bridge, and more |
| **Pure C++ GDExtension** | No external dependencies — runs inside the editor process with native performance |
| **Main-thread only** | No threading, no locks, no synchronization primitives needed |
| **Streamable HTTP** | MCP 2026-07-28 protocol on port 9600, supports GET (SSE stream), POST, OPTIONS — no sessions |
| **Runtime bridge** | TCP bridge (port 9601) to query and control running game instances |
| **X-macro registration** | No codegen, no build steps — add a tool by creating one .hpp file |
| **SDK layer** | GDScript and C# API for creating custom MCP tools |
| **Built-in test engine** | YAML-based pipeline testing framework with assertions and disk verification |
| **Undo/Redo support** | Tools integrate with Godot's EditorUndoRedoManager |
| **ClassDB-powered** | Documentation tools query Godot's own runtime class database — zero maintenance |
| **11 client config generators** | Generate configuration for opencode, Cursor, Claude Code, VS Code Copilot, Cline, and more |

## Architecture at a Glance

`
AI Client (Claude Code / Cline / Cursor / opencode …)
    │  Streamable HTTP :9600
    ▼
Godot Editor Process
    ├── McpEditorPlugin — plugin lifecycle + _process() pump
    ├── HttpServer — HTTP/1.1 + SSE
    ├── McpHandler — JSON-RPC 2.0 → MCP
    ├── HandlerRegistry — 164 ITool dispatch + search engine
    ├── SDK → McpToolRegistry — custom tool registration
    ├── RuntimeBridge — TCP :9601 → game process
    └── TestEngine — YAML pipeline testing
`

## Quick Numbers

| Metric | Value |
|--------|-------|
| Built-in tools | 164 |
| Registration files | 4 X-macro .hpp files |
| SDK classes | 2 (McpToolDefinition + McpToolRegistry) |
| Test YAML files | 26 |
| HTTP port | 9600 (configurable) |
| Bridge port | 9601 (configurable) |
| Godot version | 4.6+ |
| godot-cpp | 10.0.0-rc1 |
