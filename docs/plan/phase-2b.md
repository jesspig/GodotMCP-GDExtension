# Phase 2b — Scene Management

> **Status**: ✅ Fully shipped
> **Merge commit**: `12fb1431` (feature/scene_manager → develop)
> **Tag**: `v0.1.0` at `d1ee1fb3`

> **Archive note**: This file is kept for traceability. All planned sub-phases (2b.1–2b.5)
> have been implemented and shipped with 125 tools in 17 handler groups.
> Only 2b.6 (e2e tests) and 2b.7 (documentation sync) remain pending.
> When those two are done, delete this file and update `roadmap.md`.

## What shipped

| # | Sub-phase | Tools | Status |
|---|-----------|-------|--------|
| 2b.1 | Scene Management | `scene.rs` (16) + `node.rs` (22) = 38 | ✅ Shipped |
| 2b.2 | Script Management | `script_gd.rs` (5) + `script_cs.rs` (6) + `search.rs` (3) + `lsp/` module | ✅ Shipped |
| 2b.3 | Editor Control | `editor_control.rs` (6) + server-side `godot_editor_*` (3) | ✅ Shipped |
| 2b.4 | Project Management | `project_settings.rs` (7) + `project_settings_ext.rs` (10) + `input_map.rs` (4) + `plugin_management.rs` (2) | ✅ Shipped |
| 2b.5 | Server registry sync | 125 tools visible in `list_tools` | ✅ Shipped |
| 2b.6 | e2e tests | 5 representative tools | ⏳ Not started |
| 2b.7 | Documentation sync | parameter and response examples per tool | ⏳ Not started |

**Total**: 125 tools across 17 gdext command handler groups + 3 server-side editor control tools. All registered in `tool_registry.rs` and routed through `route_tool_call`.

## Infrastructure shipped

- Cross-thread logging: `log_info`/`log_warn`/`log_error` → `mpsc` channel → `drain_to_console()` via pump; `eprintln!` mirror
- Main-thread pump: `Callable::from_fn` on `SceneTree::process_frame` — solves `bind_mut` re-entrancy (gdext issue #338)
- `j2v`/`v2j` JSON↔Variant helpers (Vector2/3/4, Color, Rect2, Quaternion, Resource)
- `resolve_node(root, path)` for root aliases (`""`, `"."`, `"/"`, root name)
- `lsp/` module: TCP-based GDScript LSP validation via short-lived connections
- GDScript/C# built-in templates with placeholder substitution
- C# solution generated in Rust directly (no second Godot process)
- `package_addons.py` rewritten with flags; CI pipeline (fmt + clippy + build + test)
- Wiki restructure (`.repo_wiki/`), bilingual README, AGENTS.md, License

## Remaining gap: 2b.6 — e2e tests (⏳ Not started)

5 representative tools: mock WS server + real server process.

## Remaining gap: 2b.7 — Documentation sync (⏳ Not started)

Parameter and response examples per tool.
