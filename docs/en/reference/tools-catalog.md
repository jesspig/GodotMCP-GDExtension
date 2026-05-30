# Tools Catalog

GodotMCP provides **122 editor operation tools**, distributed across 16 active registered groups. Complete list organized by category.

## Meta Operations (3)

| Tool | Description |
|------|-------------|
| `ping` | Check connection status with the Godot editor |
| `get_engine_version` | Get the Godot engine version |
| `get_plugin_version` | Get the GodotMCP plugin version |

## Scene Management (16)

| Tool | Description |
|------|-------------|
| `create_scene` | Create a new scene file |
| `open_scene` | Open a scene file in the editor |
| `save_scene` | Save the current edited scene |
| `save_scene_as` | Save the current scene to a new path |
| `save_all_scenes` | Save all open scenes |
| `close_scene` | Close the current editor scene tab |
| `reload_scene` | Reload a scene from disk |
| `delete_scene` | Delete a scene file |
| `rename_scene` | Rename/move a scene file |
| `branch_to_scene` | Convert a node branch to a scene file |
| `scene_to_branch` | Convert an instanced scene to a local branch |
| `instantiate_scene` | Instantiate a child scene |
| `get_open_scenes` | List all open scene file paths |
| `get_open_scene_roots` | List root node info of all open scenes |
| `mark_scene_unsaved` | Mark the current scene as unsaved |
| `is_scene_dirty` | Check if the current scene has unsaved changes |

## Node Operations (21)

| Tool | Description |
|------|-------------|
| `create_node` | Create a new node |
| `delete_node` | Delete a node |
| `rename_node` | Rename a node |
| `duplicate_node` | Duplicate a node |
| `move_node` | Move a node to a new parent |
| `reset_parent` | Reset parent (reparent) |
| `set_as_root` | Set a node as the scene root |
| `get_scene_tree` | Get the current scene node tree |
| `get_node_path` | Get a node path |
| `get_node_info` | Get complete node info (type, script, visibility, groups, etc.) |
| `get_property_list` | Get all properties of a node |
| `get_property` | Get a specific property value of a node |
| `set_property` | Set a node property value |
| `batch_set_property` | Batch set the same property on multiple nodes |
| `attach_script` | Attach a script to a node |
| `detach_script` | Detach a script from a node |
| `add_node_to_group` | Add a node to a group |
| `remove_node_from_group` | Remove a node from a group |
| `set_node_transform_2d` | Set 2D position + rotation + scale in one action |
| `set_node_transform_3d` | Set 3D position + rotation + scale in one action |
| `get_script_variables` | List all @export variables on a script |

## 2D Properties (21)

| Tool | Description |
|------|-------------|
| `get_node_position` | Get node 2D position |
| `set_node_position` | Set node 2D position |
| `get_node_rotation` | Get node rotation (degrees) |
| `set_node_rotation` | Set node rotation (degrees) |
| `get_node_scale` | Get node scale |
| `set_node_scale` | Set node scale |
| `get_node_visible` | Get node visibility |
| `set_node_visible` | Set node visibility |
| `get_node_modulate` | Get node modulate color |
| `set_node_modulate` | Set node modulate color |
| `get_node_z_index` | Get node Z index |
| `set_node_z_index` | Set node Z index |
| `get_node_text` | Get node text |
| `set_node_text` | Set node text |
| `get_node_collision_layer` | Get collision layer |
| `set_node_collision_layer` | Set collision layer |
| `get_node_collision_mask` | Get collision mask |
| `set_node_collision_mask` | Set collision mask |
| `get_node_texture` | Get node texture property |
| `set_node_texture` | Set node texture property |
| `set_node_unique_name` | Set node unique name (% prefix) |

## 3D Properties (6)

| Tool | Description |
|------|-------------|
| `get_node_position_3d` | Get Node3D position |
| `set_node_position_3d` | Set Node3D position |
| `get_node_rotation_3d` | Get Euler angle rotation (degrees) |
| `set_node_rotation_3d` | Set Euler angle rotation (degrees) |
| `get_node_scale_3d` | Get Node3D scale |
| `set_node_scale_3d` | Set Node3D scale |

## Collision Shapes (2)

| Tool | Description |
|------|-------------|
| `add_circle_collision` | Add a circle collision shape (CollisionShape2D + CircleShape2D) |
| `add_rectangle_collision` | Add a rectangle collision shape (CollisionShape2D + RectangleShape2D) |

## Node Search (4)

| Tool | Description |
|------|-------------|
| `find_nodes_by_name` | Search nodes by name substring |
| `find_nodes_by_type` | Search nodes by exact node type |
| `find_nodes_by_group` | Search nodes by group name |
| `find_nodes_by_script` | Search nodes by script path |

## GDScript (5)

| Tool | Description |
|------|-------------|
| `create_gdscript` | Create a GDScript file |
| `read_gdscript` | Read GDScript file source |
| `edit_gdscript` | Edit GDScript file source |
| `validate_gdscript` | Validate GDScript syntax via Godot LSP |
| `list_gdscripts` | List all GDScript files in the project |

## C# Scripts (6, not yet registered)

| Tool | Description |
|------|-------------|
| `csharp_create_solution` | Create C# Solution file |
| `csharp_build` | Build C# project |
| `create_csharp_script` | Create a C# script file |
| `read_csharp_script` | Read C# script file source |
| `edit_csharp_script` | Edit C# script file source |
| `list_csharp_scripts` | List all C# script files in the project |

## Script Helpers (3)

| Tool | Description |
|------|-------------|
| `call_method` | Call a method on a node |
| `get_variable` | Read an @export variable value |
| `set_variable` | Write an @export variable value |

## Project Settings (7)

| Tool | Description |
|------|-------------|
| `get_project_setting` | Read a project setting |
| `set_project_setting` | Write a project setting |
| `set_main_scene` | Set the main scene |
| `list_autoloads` | List all Autoload singletons |
| `add_autoload` | Add an Autoload singleton |
| `remove_autoload` | Remove an Autoload singleton |
| `list_scenes` | List all scene files in the project |

## Project Settings Extensions (10)

| Tool | Description |
|------|-------------|
| `get_display_settings` | Get display/window settings |
| `set_display_settings` | Set display/window settings |
| `get_project_info` | Get project basic info |
| `set_project_info` | Set project basic info |
| `get_physics_settings` | Get physics engine settings |
| `set_physics_settings` | Set physics engine parameters |
| `get_rendering_settings` | Get rendering settings |
| `set_rendering_settings` | Set rendering parameters |
| `get_layer_names` | Get physics/navigation/render layer names |
| `set_layer_names` | Set physics/navigation/render layer names |

## Editor Control (7)

| Tool | Description |
|------|-------------|
| `get_editor_info` | Get editor info (engine version, path, scale, language) |
| `play_current_scene` | Play the current scene |
| `play_main_scene` | Play the main scene |
| `stop_scene` | Stop the running scene |
| `is_scene_playing` | Check if a scene is playing |
| `refresh_filesystem` | Refresh the editor filesystem scan |
| `godot_editor_restart` | Restart the Godot editor |

## Input Mapping (4)

| Tool | Description |
|------|-------------|
| `list_input_actions` | List input actions and their bound events |
| `add_input_action` | Add a new input action |
| `remove_input_action` | Remove an input action |
| `set_input_action_events` | Set events for an input action |

## Search (3)

| Tool | Description |
|------|-------------|
| `find_in_file` | Search for text or regex in a single file |
| `search_project` | Full-text project search |
| `find_and_replace` | Project-level find and replace |

## Plugin Management (2)

| Tool | Description |
|------|-------------|
| `list_plugins` | List all plugins and their enabled status |
| `set_plugin_enabled` | Enable or disable an editor plugin |

## Undo/Redo (2)

| Tool | Description |
|------|-------------|
| `undo` | Undo the last operation |
| `redo` | Redo the last operation |
