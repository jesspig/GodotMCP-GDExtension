# Development Plan

> **The forward-looking roadmap.** This directory captures what we *intend* to build, not what already exists.
>
> For "what exists today" go to [`.repo_wiki/`](../../.repo_wiki/index.md) — that's the verified reflection of the codebase. This `docs/plan/` is the **diff against that reality**: gaps to close, features to add, decisions to revisit.
>
> Every plan entry is written so a future agent (or me, six months from now) can pick it up without re-deriving the context.

## What's already done — quick anchor

So the plan below is read in the right frame:

**Phase 1 — Foundations** ✅ (`ad9c2eb5`..`e15e5183`)
- Cargo workspace (3 crates), `IpcRequest`/`IpcResponse`/`ToolManifest` protocol types
- GDExtension `EditorPlugin` + `PluginState` + WebSocket server on `:9500`
- MCP server with rmcp stdio, `GodotBridge` (id→oneshot), `GodotMcpHandler`
- 4 meta tools (ping, versions), lazy-connect, CI pipeline, packaging script

**Phase 2a — MVP Integration** ✅ (`5c68d32a`)
- `ToolCallParams`, `MainThreadDispatcher`, `ToolRegistry` with enable/disable
- `CommandHandler` trait + `MetaCommands`/`SceneCommands` + `route_tool_call` dispatcher
- `broadcast_tx` notification channel + dynamic `list_tools`
- Dock UI: 4-panel `VBoxContainer` skeleton (status bar, tool manager, integration, settings)

**Phase 2b — Scene Management** 🛠 Partially shipped (`12fb1431`, tagged `v0.1.0`) — see [`phase-2b.md`](phase-2b.md)
- ✅ 2b.1 Scene Management (10): `get_scene_tree`, `create_node`, `delete_node`, `modify_node_property`, `get_node_properties`, `move_node`, `duplicate_node`, `rename_node`, `set_node_script`, `find_nodes` + 21 utility tools (scene-file ops, editor tabs)
- ⏳ 2b.2 Script Management (13): GDScript 子组 (6) + C# 子组 (5) + 通用搜索 (2)，含 LSP 接入 validate_gdscript
- ⏳ 2b.3 Editor Control (7): `play`, `pause`, `stop`, `get_console`, `clear_console`, `refresh_project`, `execute_menu_item`
- ⏳ 2b.4 Project Management (6): `get_project_settings`, `update_project_settings`, `get_input_map`, `configure_input_map`, `list_scenes`, `run_tests`
- ⏳ 2b.6 e2e tests (5 representative tools), 2b.7 documentation sync
- Shipped: logging pump, j2v/v2j, resolve_node, server registry 4→35, wiki restructure

Full inventory: [`.repo_wiki/overview/architecture.md`](../../.repo_wiki/overview/architecture.md).

## Roadmap at a glance

| Phase | Theme | Status | Files |
|-------|-------|--------|-------|
| 1 | Foundations (workspace, plugin, WS server) | ✅ Shipped | — |
| 2a | Server routing & IPC (35 tools, bridge, pump) | ✅ Shipped | — |
| 2b | Scene Management (partial: 2b.1 done, 2b.2–2b.7 pending) | 🛠 Partial | [`phase-2b.md`](phase-2b.md) |
| 3 | Polish the editor experience | ⏳ Not started | [`roadmap.md`](roadmap.md), [`phase-3-dock-ui.md`](phase-3-dock-ui.md) |
| 4 | HTTP transport for the long-tail clients | ⏳ Not started | [`roadmap.md`](roadmap.md), [`phase-4-http-transport.md`](phase-4-http-transport.md) |
| 5 | Beyond the editor: runtime / asset / project / script / debug tool groups | ⏳ Not started | [`roadmap.md`](roadmap.md), [`phase-5-tool-expansion.md`](phase-5-tool-expansion.md) |
| 6 | Resilience: heartbeat, reconnect, multi-client, audit log | ⏳ Not started | [`roadmap.md`](roadmap.md), [`phase-6-resilience.md`](phase-6-resilience.md) |
| ongoing | Open ADR / questions | — | [`open-questions.md`](open-questions.md) |

## How to evolve this plan

1. When you start a phase: link the relevant page from `roadmap.md`, drop notes into the phase's own file as you discover constraints.
2. When you finish work that closes a gap listed here: **delete the gap** from the plan and add the matching verified description to [`.repo_wiki/`](../../.repo_wiki/index.md). The plan is for things that haven't happened yet.
3. When a phase is fully shipped: archive the page (or delete it) and update `roadmap.md`. Don't keep "Done ✅" entries here — that's what `.repo_wiki/log.md` is for.

Status legend used inside phase files: `⏳ Not started` · `🚧 In progress` · `🚫 Blocked` · `✅ Shipped` (then move to the wiki).
