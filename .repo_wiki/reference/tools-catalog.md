# Tool Catalog

> All 35 tools registered as of 2026-05-22. Source of truth: `crates/server/src/tool_registry.rs::register_defaults` (schemas) and `crates/gdext/src/commands/{meta,scene}.rs` (implementations).

Argument shapes are JSON; vector/colour/rect values follow [`modules/scene-commands.md`](../modules/scene-commands.md) conventions (e.g. `Vector2 = {"x": …, "y": …}`, `Color = {"r": …, "g": …, "b": …, "a": …}`, `Resource = "res://..."` or `{"resource_path": "res://..."}`).

Most node-targeting tools accept the special root forms (`""`, `"."`, `"/"`, `"/root"`, root-name) via `resolve_node`.

## Meta (4)

| Tool | Args | Returns | Notes |
|------|------|---------|-------|
| `ping` | `{}` | `{"message": "pong"}` | Round-trips through the GDExtension. Use to confirm the WS bridge is alive. |
| `get_engine_version` | `{}` | `{"engine_version": "4.6.2"}` | Read from `Engine::singleton().get_version_info()` at plugin boot. |
| `get_plugin_version` | `{}` | `{"plugin_version": "0.1.0"}` | `CARGO_PKG_VERSION` of the gdext crate. |
| `get_server_version` | `{}` | `"0.1.0"` | **Short-circuited on the server**; never reaches the GDExtension. `CARGO_PKG_VERSION` of the server crate. |

## Scene: Read (4)

| Tool | Required args | Optional args | Returns |
|------|---------------|---------------|---------|
| `get_scene_tree` | — | `max_depth: int` (default 10) | Recursive `{name, type, path, child_count, children: [...]}`. |
| `get_node_path` | `node_path: string` | — | `{"path": "/root/Main/Player"}`. Useful to translate human-readable name into a stable absolute path. |
| `get_property_list` | `node_path: string` | — | `{"properties": [{name, type, ...}, ...]}` — every editable property reported by the node. |
| `get_property` | `node_path: string`, `property: string` | — | `{"value": <typed JSON via v2j>}`. |

## Scene: Node write (6)

| Tool | Required args | Optional args | Returns |
|------|---------------|---------------|---------|
| `create_node` | `parent_path: string`, `node_type: string`, `name: string` | — | `{"path": "/root/Main/<name>"}`. `node_type` is any class name `ClassDb::singleton().instantiate(...)` accepts (`Node2D`, `Sprite2D`, `CharacterBody2D`, …). |
| `delete_node` | `node_path: string` | — | `{"deleted": "<path>"}`. Cannot delete the scene root inside the editor (Godot constraint). |
| `rename_node` | `node_path: string`, `new_name: string` | — | `{"new_path": "<new full path>"}`. |
| `set_property` | `node_path: string`, `property: string`, `value: any` | — | `{"node_path", "property", "value"}`. `value` is converted via `j2v` — accepts the full typed shapes. |
| `duplicate_node` | `node_path: string` | — | `{"new_path": "<path>"}`. The copy gets a `_copy` suffix and is added as a sibling. |
| `move_node` | `node_path: string`, `new_parent_path: string` | `position_index: int` | `{"new_path"}`. Reparents under a new parent; ordering optional. |

## Scene: Script + search (3)

| Tool | Required args | Returns |
|------|---------------|---------|
| `attach_script` | `node_path: string`, `script_path: string` (must be `res://...`) | `{"node_path", "script_path"}`. Uses `try_load::<Script>` + `node.set("script", …)`. |
| `detach_script` | `node_path: string` | `{"node_path", "detached": true}`. |
| `find_nodes` | `query: string`, `search_method?: string` | `{"matches": [{name, path}, ...]}`. Walks the edited scene tree by name. |

## Scene: Scene-file ops (6)

| Tool | Required args | Optional args | Returns |
|------|---------------|---------------|---------|
| `create_scene` | `path: string` (`res://.../foo.tscn`) | — | `{"created": "<path>"}`. Writes an empty `PackedScene` with a placeholder root. |
| `delete_scene` | `path: string` | — | `{"deleted": "<path>"}`. Uses `DirAccess`. |
| `rename_scene` | `source_path: string`, `dest_path: string` | — | `{"renamed": "<dest>"}`. |
| `branch_to_scene` | `node_path: string`, `scene_path: string` | — | `{"scene_path", "node_path"}`. Packs the node subtree via `PackedScene::pack(node)` then `godot::tools::save`. The original node is untouched (use `instantiate_scene` afterwards to swap it). |
| `scene_to_branch` | `node_path: string` | — | `{"node_path", "converted": true, "source_scene"}`. Only works when `node.get_scene_file_path()` is non-empty (i.e. the node *is* an instantiated scene). |
| `instantiate_scene` | `scene_path: string`, `parent_path: string` | `name: string` | `{"path": "<full path>", "parent": "<parent path>"}`. `parent_path` accepts the root-form aliases (`""`, `"."`, etc.) — that's why this tool finally works at the scene root. |

## Scene: Advanced (3)

| Tool | Required args | Returns |
|------|---------------|---------|
| `reset_parent` | `node_path: string`, `new_parent_path: string` | `{"node_path", "new_parent"}`. Uses `node.reparent(new_parent)`. |
| `set_as_root` | `node_path: string` | `{"root": "<path>"}`. Calls `tree.set_edited_scene_root(&node)`. Marks scene unsaved. |
| `batch_set_property` | `node_paths: string[]`, `property: string`, `value: any` | `{"results": [{path, ok|error}, ...]}`. Same `j2v` conversion as `set_property`. |

## Scene: Editor scene tabs (9)

| Tool | Required args | Optional args | Returns |
|------|---------------|---------------|---------|
| `open_scene` | `scene_path: string` | `set_inherited: bool` (default `false`) | `{"opened": "<path>"}`. Opens or focuses an existing tab. **Use this when no scene is open** — without it, every node tool returns "No scene open". |
| `close_scene` | — | — | `{"closed": true}` on `Error::OK`, otherwise `{"error": "..."}`. Closes the current tab. |
| `save_scene` | — | — | `{"saved": "<path>"}`. Fails if the current scene has no path yet — use `save_scene_as` first in that case. |
| `save_scene_as` | `scene_path: string` | `with_preview: bool` (default `true`) | `{"saved": "<path>"}`. Builder form `save_scene_as_ex().with_preview(...).done()`. |
| `save_all_scenes` | — | — | `{"count": <number open>}`. Returns the count of currently-open scenes before saving (used as a hint, not a strict success count). |
| `reload_scene` | `scene_path: string` | — | `{"reloaded": "<path>"}`. The gdext API returns `()`, not `Error`, so this tool always reports success. |
| `get_open_scenes` | — | — | `{"scenes": ["res://...", ...]}`. |
| `get_open_scene_roots` | — | — | `{"roots": [{name, path, scene_file_path, class}, ...]}`. |
| `mark_scene_unsaved` | — | — | `{"marked": true}`. Adds the `*` to the tab — useful when you've mutated state outside of `set_property` and want to force a save prompt. |

## Tool-count assertions

The test suite hard-codes these:

- Default registry: **35** total, all enabled.
- After disabling 1: **34** enabled.
- After disabling 2: **33** enabled.
- After registering a custom tool: **36** total.

When you change the catalog, update `crates/server/src/handler.rs::tests` and `crates/server/src/tool_registry.rs::tests` together.

## Behaviour that's easy to get wrong

- **No active scene → most tools fail with `"No scene open"`**. Always pair an `open_scene` first (or `create_scene` → `open_scene`) in a fresh test session.
- **`scene_to_branch` requires an instantiated scene**: if `node.get_scene_file_path()` is empty, returns `"Node '<path>' is not an instanced scene (no scene_file_path). scene_to_branch only converts nodes that were instantiated…"`. Use `branch_to_scene` for the inverse use case (snapshot a regular node as a new scene file).
- **Node paths embed session-specific IDs** like `@EditorNode@18094` — they're not stable across sessions. Always re-fetch via `get_scene_tree` / `find_nodes` before chaining tools.
