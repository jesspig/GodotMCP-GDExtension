# Tools Catalog

> All 99 MCP tools with descriptions, parameters, and return values. Organized by 13 handler groups + 3 server-side tools.

## Group Summary

| Group | Count | Route |
|-------|-------|-------|
| [Editor Control](#editor-control-server-side-3) | 3 | server-side |
| [MetaCommands](#metacommands-4) | 4 | gdext |
| [NodeCommands](#nodecommands-17) | 17 | gdext |
| [PropertyCommands](#propertycommands-19) | 19 | gdext |
| [CollisionCommands](#collisioncommands-2) | 2 | gdext |
| [FindCommands](#findcommands-4) | 4 | gdext |
| [ScriptHelpersCommands](#scripthelperscommands-3) | 3 | gdext |
| [ProjectSettingsCommands](#projectsettingscommands-3) | 3 | gdext |
| [SceneCommands](#scenecommands-15) | 15 | gdext |
| [ScriptGdCommands](#scriptgdcommands-5) | 5 | gdext |
| [ScriptCsCommands](#scriptcscommands-6) | 6 | gdext |
| [SearchCommands](#searchcommands-3) | 3 | gdext |
| [UndoCommands](#undocommands-2) | 2 | gdext |
| [Property3dCommands](#property3dcommands-6) | 6 | gdext |
| [NodeConvenience](#nodeconvenience-4) | 4 | gdext |
| [SceneInfo](#sceneinfo-1) | 1 | gdext |
| **Total** | **99** | |

---

## Editor Control (Server-Side, 3)

Requires `GODOT_PATH` env var.

### `godot_editor_open`
Launch Godot editor and open project.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `project_path` | string | no | `godot/` |

**Returns**: `{"status": "ok"}`

### `godot_editor_close`
Close Godot editor process.

**Returns**: `{"status": "ok"}`

### `godot_editor_restart`
Restart Godot editor.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `project_path` | string | no | `godot/` |

**Returns**: `{"status": "ok"}`

---

## MetaCommands (4)

### `ping`
Check connection to Godot editor.

**Returns**: `{"status": "ok", "message": "pong"}`

### `get_engine_version`
Get Godot engine version.

**Returns**: `{"major": 4, "minor": 6, "patch": 0, "string": "4.6"}`

### `get_plugin_version`
Get Godot MCP plugin version.

**Returns**: `{"version": "x.y.z"}`

### `get_server_version`
Get godot-mcp-server version.

**Returns**: `{"version": "x.y.z"}`

---

## NodeCommands (17)

### `create_node`
Create new node.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `parent_path` | string | yes | |
| `node_type` | string | yes | |
| `name` | string | yes | |

**Returns**: `{"status": "ok", "name": "..."}`

### `delete_node`
Delete node.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"status": "ok"}`

### `duplicate_node`
Duplicate node.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"status": "ok", "duplicated_name": "..."}`

### `rename_node`
Rename node.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `new_name` | string | yes |

**Returns**: `{"status": "ok"}`

### `move_node`
Move node to new parent.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `new_parent_path` | string | yes |

**Returns**: `{"status": "ok"}`

### `reset_parent`
Reparent node.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `new_parent_path` | string | yes |

**Returns**: `{"status": "ok"}`

### `set_as_root`
Set node as scene root.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"status": "ok"}` — verifies root was actually changed.

### `attach_script`
Attach script to node.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `script_path` | string | yes |

**Returns**: `{"status": "ok"}`

### `detach_script`
Detach script from node.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"status": "ok"}`

### `add_node_to_group`
Add node to group.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `group` | string | yes |

**Returns**: `{"status": "ok"}`

### `remove_node_from_group`
Remove node from group.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `group` | string | yes |

**Returns**: `{"status": "ok"}`

### `set_node_unique_name`
Set node unique name (% prefix).

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `unique` | boolean | yes |

**Returns**: `{"status": "ok"}`

### `get_scene_tree`
Get current scene node tree.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `max_depth` | integer | no | unlimited |

**Returns**: `{"nodes": [{"name":"...", "type":"...", "children":[...]}]}`

### `get_node_path`
Get node path.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"path": "..."}`

### `get_node_info`
Get full node information.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"type":"...", "script":"...", "visible":true, "groups":[], "child_count":3, "owner":"...", "unique_name":false, "is_instance":false}`

### `branch_to_scene`
Save node branch as scene file.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `scene_path` | string | yes |

**Returns**: `{"status": "ok", "scene_path": "..."}`

### `scene_to_branch`
Convert instanced scene to local branch.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"status": "ok"}`

---

## PropertyCommands (19)

### `get_property`
Get node property.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `property` | string | yes |

**Returns**: `{"value": ...}`

### `set_property`
Set node property (generic fallback).

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `property` | string | yes |
| `value` | any | yes |

**Returns**: `{"status": "ok"}`

### `get_property_list`
Get all node properties.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"properties": [{"name":"...", "type":..., "hint":...}]}`

### `batch_set_property`
Batch set node property.

| Param | Type | Required |
|-------|------|----------|
| `node_paths` | array[string] | yes |
| `property` | string | yes |
| `value` | any | yes |

**Returns**: `{"status": "ok"}`

### `get_node_position`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"x": 100, "y": 200}`

### `set_node_position`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `x` | number | yes |
| `y` | number | yes |

**Returns**: `{"status": "ok"}`

### `get_node_rotation`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"degrees": 45.0}`

### `set_node_rotation`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `degrees` | number | yes |

**Returns**: `{"status": "ok"}`

### `get_node_scale`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"x": 1.0, "y": 1.0}`

### `set_node_scale`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `x` | number | yes |
| `y` | number | yes |

**Returns**: `{"status": "ok"}`

### `get_node_modulate`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0}`

### `set_node_modulate`
| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `r` | number | no | 1.0 |
| `g` | number | no | 1.0 |
| `b` | number | no | 1.0 |
| `a` | number | no | 1.0 |

**Returns**: `{"status": "ok"}`

### `get_node_visible`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"visible": true}`

### `set_node_visible`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `visible` | boolean | yes |

**Returns**: `{"status": "ok"}`

### `get_node_text`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"text": "Hello"}`

### `set_node_text`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `text` | string | yes |

**Returns**: `{"status": "ok"}`

### `get_node_z_index`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"z_index": 0}`

### `set_node_z_index`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `z_index` | integer | yes |

**Returns**: `{"status": "ok"}`

### `call_method`
Call node method.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `method` | string | yes |
| `args` | array | no | `[]` |

**Returns**: `{"result": ...}`

---

## CollisionCommands (2)

### `add_circle_collision`
Add circular collision shape.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `radius` | number | no | auto |

**Returns**: `{"status": "ok", "mode": "create|detect_existing"}`

### `add_rectangle_collision`
Add rectangle collision shape.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `width` | number | no | auto |
| `height` | number | no | auto |

**Returns**: `{"status": "ok", "mode": "create|detect_existing"}`

---

## FindCommands (4)

### `find_nodes_by_name`
Search nodes by name substring (case-sensitive, contains).

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `pattern` | string | yes |
| `max_results` | integer | no | unlimited |

**Returns**: `{"matches": [{"name":"...","path":"..."}]}`

### `find_nodes_by_type`
Search nodes by exact type.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_class` | string | yes |
| `max_results` | integer | no | unlimited |

**Returns**: `{"matches": [{"name":"...","path":"..."}]}`

### `find_nodes_by_group`
Search nodes by group.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `group` | string | yes |
| `max_results` | integer | no | unlimited |

**Returns**: `{"matches": [{"name":"...","path":"..."}]}`

### `find_nodes_by_script`
Search nodes by script path.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `script_path` | string | yes |
| `max_results` | integer | no | unlimited |

**Returns**: `{"matches": [{"name":"...","path":"..."}]}`

---

## ScriptHelpersCommands (3)

### `get_script_variables`
List all @export variables on node script.

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"variables": [{"name":"speed","type":"int","value":100}]}`

### `get_variable`
Read node script variable (editor mode: @export only).

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `variable` | string | yes |

**Returns**: `{"value": ...}`

### `set_variable`
Write node script variable (editor mode: @export only).

| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `variable` | string | yes |
| `value` | any | yes |

**Returns**: `{"status": "ok"}`

---

## ProjectSettingsCommands (3)

### `get_project_setting`
Read project setting.

| Param | Type | Required |
|-------|------|----------|
| `property` | string | yes |

**Returns**: `{"value": ...}`

### `set_project_setting`
Write project setting. Auto-saves via `ProjectSettings::save()`.

| Param | Type | Required |
|-------|------|----------|
| `property` | string | yes |
| `value` | any | yes |

**Returns**: `{"status": "ok"}` — with `"warning": "Save failed: ..."` field if save fails.

### `set_main_scene`
Set main scene.

| Param | Type | Required |
|-------|------|----------|
| `scene_path` | string | yes |

**Returns**: `{"status": "ok"}`

---

## SceneCommands (15)

### `create_scene`
Create new scene file.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `path` | string | yes |
| `root_type` | string | no | `Node` |
| `root_name` | string | no | filename |

**Returns**: `{"status": "ok", "scene_path": "..."}`

### `open_scene`
Open scene in editor.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `scene_path` | string | yes |
| `set_inherited` | boolean | no | `false` |

**Returns**: `{"status": "ok"}`

### `save_scene`
Save current scene.

**Returns**: `{"status": "ok"}`

### `save_scene_as`
Save current scene as new path.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `scene_path` | string | yes |
| `with_preview` | boolean | no | `false` |

**Returns**: `{"status": "ok"}`

### `save_all_scenes`
Save all open scenes.

**Returns**: `{"status": "ok"}`

### `close_scene`
Close current scene tab.

**Returns**: `{"status": "ok"}`

### `reload_scene`
Reload scene from disk.

| Param | Type | Required |
|-------|------|----------|
| `scene_path` | string | yes |

**Returns**: `{"status": "ok"}`

### `rename_scene`
Rename/move scene file.

| Param | Type | Required |
|-------|------|----------|
| `source_path` | string | yes |
| `dest_path` | string | yes |

**Returns**: `{"status": "ok"}` — error if target is open but not active tab.

### `delete_scene`
Delete scene file.

| Param | Type | Required |
|-------|------|----------|
| `path` | string | yes |

**Returns**: `{"status": "ok"}`

### `is_scene_dirty`
Check if current scene has unsaved changes.

**Returns**: `{"dirty": false}`

### `mark_scene_unsaved`
Mark current scene as unsaved.

**Returns**: `{"status": "ok"}`

### `instantiate_scene`
Instantiate child scene.

| Param | Type | Required |
|-------|------|----------|
| `scene_path` | string | yes |
| `parent_path` | string | yes |

**Returns**: `{"status": "ok", "name": "..."}`

### `get_open_scenes`
List all open scene file paths.

**Returns**: `{"scenes": ["res://scene1.tscn", "res://scene2.tscn"]}`

### `get_open_scene_roots`
List open scene root node info.

**Returns**: `{"roots": [{"path":"res://s1.tscn","type":"Node2D","name":"Root1"}]}`

### `set_node_transform_2d`
Set 2D node position + rotation + scale (single undo action).

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `x` | number | no | 0 |
| `y` | number | no | 0 |
| `degrees` | number | no | 0 |
| `scale_x` | number | no | 1 |
| `scale_y` | number | no | 1 |

**Returns**: `{"status": "ok"}`

### `set_node_transform_3d`
Set 3D node position + rotation + scale (single undo action).

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `x` | number | no | 0 |
| `y` | number | no | 0 |
| `z` | number | no | 0 |
| `rot_x` | number | no | 0 |
| `rot_y` | number | no | 0 |
| `rot_z` | number | no | 0 |
| `scale_x` | number | no | 1 |
| `scale_y` | number | no | 1 |
| `scale_z` | number | no | 1 |

**Returns**: `{"status": "ok"}`

---

## ScriptGdCommands (5)

### `create_gdscript`
Create GDScript file.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `path` | string | yes |
| `base_class` | string | yes |
| `class_name` | string | no | none |
| `template` | string | no | default |
| `overwrite` | boolean | no | `false` |

**Returns**: `{"status": "ok", "path": "..."}`

### `edit_gdscript`
Edit GDScript source.

| Param | Type | Required |
|-------|------|----------|
| `path` | string | yes |
| `source` | string | yes |

**Returns**: `{"status": "ok"}`

### `read_gdscript`
Read GDScript source.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `path` | string | yes |
| `from_editor` | boolean | no | `false` |

**Returns**: `{"source": "..."}`

### `list_gdscripts`
List project GDScript files.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `root` | string | no | `res://` |
| `include_addons` | boolean | no | `false` |
| `max_results` | integer | no | unlimited |

**Returns**: `{"scripts": ["res://script1.gd", "res://script2.gd"]}`

### `validate_gdscript`
Validate GDScript syntax (via Godot LSP).

| Param | Type | Required |
|-------|------|----------|
| `path` | string | yes |

**Returns**: `{"valid": true}` or `{"valid": false, "errors": [...]}`

**Note**: Requires Editor Settings → Network → Language Server → Enable = ON. Creates a temporary tokio runtime.

---

## ScriptCsCommands (6)

### `csharp_create_solution`
Create C# Solution.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `enable_nativeaot` | boolean | no | `false` |

**Returns**: `{"status": "ok", "sln_path": "...", "csproj_path": "..."}`

### `create_csharp_script`
Create C# script (requires csharp_create_solution first).

| Param | Type | Required |
|-------|------|----------|
| `path` | string | yes |
| `base_class` | string | yes |
| `overwrite` | boolean | no |

**Returns**: `{"status": "ok", "path": "..."}`

### `edit_csharp_script`
Edit C# script (requires csharp_build to compile).

| Param | Type | Required |
|-------|------|----------|
| `path` | string | yes |
| `source` | string | yes |

**Returns**: `{"status": "ok"}`, notifies EditorFileSystem after write.

### `read_csharp_script`
Read C# script source.

| Param | Type | Required |
|-------|------|----------|
| `path` | string | yes |

**Returns**: `{"source": "..."}`

### `list_csharp_scripts`
List project C# scripts.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `root` | string | no | `res://` |
| `include_addons` | boolean | no | `false` |
| `max_results` | integer | no | unlimited |

**Returns**: `{"scripts": ["res://script1.cs", "res://script2.cs"]}`

### `csharp_build`
Build C# project (calls dotnet build).

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `configuration` | string | no | `Debug` |

**Returns**: `{"status": "ok", "output": "..."}`, notifies EditorFileSystem after build.

---

## SearchCommands (3)

### `find_in_file`
Search single file (text or regex).

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `path` | string | yes |
| `pattern` | string | yes |
| `case_sensitive` | boolean | no | `false` |
| `literal` | boolean | no | `true` |
| `max_matches` | integer | no | unlimited |

**Returns**: `{"matches": [{"line": 10, "content": "...", "column": 5}]}`

### `search_project`
Full-text project search.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `pattern` | string | yes |
| `root` | string | no | `res://` |
| `extensions` | array[string] | no | `["gd","tscn","cs","res"]` |
| `case_sensitive` | boolean | no | `false` |
| `literal` | boolean | no | `true` |
| `max_files` | integer | no | 50 |
| `max_matches` | integer | no | 500 |

**Returns**: `{"results": [{"file":"res://main.gd","matches":[{"line":5, ...}]}]}`

### `find_and_replace`
Project-level find and replace.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `path` | string | yes |
| `pattern` | string | yes |
| `replacement` | string | yes |
| `case_sensitive` | boolean | no | `false` |
| `literal` | boolean | no | `true` |
| `max_replacements` | integer | no | unlimited |

**Returns**: `{"status": "ok", "replacements": 5}`, notifies EditorFileSystem after changes.

---

## UndoCommands (2)

### `undo`
Undo last action.

**Returns**: `{"status": "ok"}`

### `redo`
Redo last action.

**Returns**: `{"status": "ok"}`

---

## Property3dCommands (6)

### `get_node_position_3d`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"x": 0.0, "y": 0.0, "z": 0.0}`

### `set_node_position_3d`
| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `x` | number | no | 0 |
| `y` | number | no | 0 |
| `z` | number | no | 0 |

**Returns**: `{"status": "ok"}`

### `get_node_rotation_3d`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"x": 0.0, "y": 0.0, "z": 0.0}`

### `set_node_rotation_3d`
| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `x` | number | no | 0 |
| `y` | number | no | 0 |
| `z` | number | no | 0 |

**Returns**: `{"status": "ok"}`

### `get_node_scale_3d`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"x": 1.0, "y": 1.0, "z": 1.0}`

### `set_node_scale_3d`
| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `x` | number | no | 1 |
| `y` | number | no | 1 |
| `z` | number | no | 1 |

**Returns**: `{"status": "ok"}`

---

## NodeConvenience (4)

### `set_node_texture`
Set node texture.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `texture_path` | string | yes |
| `property` | string | no | `"texture"` |

**Returns**: `{"status": "ok"}`. Supports TextureButton properties like `texture_normal/pressed/hover`.

### `get_node_texture`
Get node texture.

| Param | Type | Required | Default |
|-------|------|----------|---------|
| `node_path` | string | yes |
| `property` | string | no | `"texture"` |

**Returns**: `{"path": "res://icon.svg"}`

### `get_node_collision_layer`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"layer": 1}` — error for non-CollisionObject2D/3D.

### `set_node_collision_layer`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `layer` | integer | yes |

**Returns**: `{"status": "ok"}` — error for non-CollisionObject2D/3D.

### `get_node_collision_mask`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |

**Returns**: `{"mask": 1}`

### `set_node_collision_mask`
| Param | Type | Required |
|-------|------|----------|
| `node_path` | string | yes |
| `mask` | integer | yes |

**Returns**: `{"status": "ok"}`

---

## SceneInfo (1)

### `get_open_scene_roots`
List all open scene root nodes.

**Returns**: `{"roots": [...]}`
