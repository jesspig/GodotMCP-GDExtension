# Development Plan

> **The forward-looking roadmap.** This directory captures what we *intend* to build, not what already exists.
>
> For "what exists today" go to [`.repo_wiki/`](../../.repo_wiki/index.md) — that's the verified reflection of the codebase. This `docs/plan/` is the **diff against that reality**: gaps to close, features to add, decisions to revisit.
>
> Every plan entry is written so a future agent (or me, six months from now) can pick it up without re-deriving the context.

## What's already done — quick anchor

So the plan below is read in the right frame:

- 35 tools end-to-end: 4 meta + 31 scene (incl. node CRUD, properties, scripts, scene-file ops, editor scene tabs).
- Two-process architecture stable: `godot-mcp-server` (rmcp stdio) ↔ WebSocket `:9500` ↔ `godot_mcp_gdext` (cdylib).
- The hard problems are solved: tokio↔main-thread `MainThreadDispatcher`, cross-thread `logging` channel, `Callable::from_fn` pump on `SceneTree::process_frame`, `j2v`/`v2j` typed JSON↔Variant.
- Right-dock UI exists as a 4-panel `VBoxContainer` skeleton. Status bar shows a hardcoded green dot. Tool toggles work end-to-end *if* a tool is in the list — but the list itself hardcodes 4 entries instead of reading the real 35.
- CI gates pass: `fmt --check`, `clippy -D warnings`, `build`, `test` (47 tests).
- `package_addons.py` produces a release-able `addons.zip`.

Full inventory: [`.repo_wiki/overview/architecture.md`](../../.repo_wiki/overview/architecture.md).

## Roadmap at a glance

| Phase | Theme | Status | Files |
|-------|-------|--------|-------|
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
