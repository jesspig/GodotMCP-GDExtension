# Tools Catalog

GodotMCP provides **149+ editor operation tools** organized by functional domain: meta operations, scene tree, scripts, filesystem, workspace/debugger, runtime bridge, and more. Tools are auto-registered via X-macro with `find_tool` search engine and progressive discovery.

## Meta Tools (6)

| Tool | Description |
|------|-------------|
| `get_info` | Get editor connection status and runtime environment info |
| `get_tools` | List all registered tools |
| `get_categories` | List tool category tree |
| `get_tool_detail` | Get full metadata for a specific tool |
| `find_tool` | Full-text tool search (name, description, category) |
| `call_tool` | Call any tool by name |

## Signal Management (4)

| Tool | Description |
|------|-------------|
| `connect_signal` | Connect a node signal |
| `disconnect_signal` | Disconnect a node signal |
| `list_signals` | List all signals of a node |
| `get_signal_connections` | Get signal connection list |

## Node Groups (4)

| Tool | Description |
|------|-------------|
| `add_to_group` | Add a node to a group |
| `remove_from_group` | Remove a node from a group |
| `get_node_groups` | Get groups a node belongs to |
| `get_nodes_in_group` | Get all nodes in a group |

## Resource Operations (7)

| Tool | Description |
|------|-------------|
| `save_resource` | Save a resource to file |
| `load_resource` | Load a resource from path |
| `new_resource` | Create a new resource |
| `duplicate_resource` | Duplicate a resource |
| `clear_resource` | Clear a node's resource reference |
| `get_resource_info` | Get resource information |

## Property Fallback (2)

| Tool | Description |
|------|-------------|
| `get_node_property` | Get any node property value (generic fallback) |
| `set_node_property` | Set any node property value (generic fallback) |

## Scene Tree Operations (24)

| Tool | Description |
|------|-------------|
| `add_node` | Create a new node |
| `delete_node` | Delete a node |
| `rename_node` | Rename a node |
| `move_node` | Move a node to a new parent |
| `duplicate_node` | Duplicate a node |
| `copy_node` | Copy a node to clipboard |
| `paste_node` | Paste a node from clipboard |
| `cut_node` | Cut a node |
| `get_scene_tree` | Get the current scene node tree |
| `save_scene` | Save the current scene |
| `new_scene` | Create a new scene |
| `change_node_type` | Change a node's type |
| `attach_script` | Attach a script to a node |
| `detach_script` | Detach a script from a node |
| `reparent_node` | Reparent a node |
| `reparent_to_new_node` | Reparent a node to a new node |
| `group_selected_nodes` | Group selected nodes |
| `make_local` | Make an instanced scene node local |
| `save_branch_as_scene` | Save a node branch as a scene file |
| `instance_child_scene` | Instance a child scene |
| `set_unique_name` | Set node unique name (% prefix) |
| `toggle_editable_children` | Toggle editable children state |
| `toggle_edit_group` | Toggle edit group state |
| `toggle_placeholder` | Toggle placeholder mode |

## Animation (5)

| Tool | Description |
|------|-------------|
| `create_animation_player` | Create an AnimationPlayer node |
| `create_animation_clip` | Create an animation clip |
| `add_animation_track` | Add an animation track |
| `set_keyframe` | Set a keyframe |
| `get_animation_info` | Get animation info |

## UI Controls (4)

| Tool | Description |
|------|-------------|
| `create_control` | Create a UI control node |
| `create_stylebox` | Create a StyleBox resource |
| `set_layout_preset` | Set control layout preset |
| `set_theme_override` | Set theme override |

## Collision Shapes (1)

| Tool | Description |
|------|-------------|
| `create_collision_shape` | Create a collision shape |

## ClassDB Documentation (8)

| Tool | Description |
|------|-------------|
| `search_docs` | Search Godot documentation |
| `get_class_info` | Get complete class info |
| `get_best_practices` | Get best practices advice |
| `get_class_list` | List all Godot classes |
| `get_inheritance_chain` | Get class inheritance chain |
| `get_property_doc` | Query property documentation |
| `get_method_doc` | Query method documentation |
| `get_enum_doc` | Query enum documentation |

## Export (2)

| Tool | Description |
|------|-------------|
| `list_export_presets` | List export presets |
| `export_project` | Export project |

## Filesystem (12)

| Tool | Description |
|------|-------------|
| `create` | Create a file |
| `create_directory` | Create a directory |
| `create_scene` | Create a scene file |
| `create_resource` | Create a resource file |
| `create_gdshader` | Create a shader file |
| `delete_file` | Delete a file |
| `move_file` | Move/rename a file |
| `copy_file` | Copy a file |
| `open_file` | Open a file in the editor |
| `list_directory` | List directory contents |
| `search_files` | Search project files |
| `save_resource_as` | Save resource as |

## Input Mapping (1)

| Tool | Description |
|------|-------------|
| `input_list_actions` | List all input actions and bound events |

## Plugin Management (3)

| Tool | Description |
|------|-------------|
| `list_plugins` | List all plugins and their status |
| `enable_plugin` | Enable a plugin |
| `disable_plugin` | Disable a plugin |

## Project Scaffolding (1)

| Tool | Description |
|------|-------------|
| `create_project` | Create a new Godot project |

## Script Tools (12)

| Tool | Description |
|------|-------------|
| `read_gd_script` | Read a GDScript file |
| `write_gd_script` | Write a GDScript file |
| `patch_gd_script` | Patch a GDScript file |
| `validate_gd_script` | Validate GDScript syntax (via LSP) |
| `list_gd_scripts` | List all GDScript files |
| `grep_scripts` | Search text in scripts |
| `glob_scripts` | Search script files by pattern |
| `read_csharp_script` | Read a C# script file |
| `write_csharp_script` | Write a C# script file |
| `patch_csharp_script` | Patch a C# script file |
| `validate_csharp_script` | Validate C# script syntax |
| `list_csharp_scripts` | List all C# script files |

## Project Settings (4)

| Tool | Description |
|------|-------------|
| `get_setting` | Read a project setting |
| `set_setting` | Write a project setting |
| `reset_setting` | Reset a project setting to default |
| `list_settings` | List all project settings |

## Shaders (3)

| Tool | Description |
|------|-------------|
| `create_shader` | Create a shader |
| `read_shader` | Read shader source |
| `apply_shader_preset` | Apply a shader preset |

## TileMap (3)

| Tool | Description |
|------|-------------|
| `get_tilemap_info` | Get TileMap info |
| `set_tilemap_cells` | Set TileMap cells |
| `erase_tilemap_cells` | Erase TileMap cells |

## Project Graph Visualization (1)

| Tool | Description |
|------|-------------|
| `get_project_graph` | Get project dependency graph |

## Workspace & Debugger (31)

### Viewport Capture (2)

| Tool | Description |
|------|-------------|
| `capture_viewport` | Capture editor viewport |
| `capture_game_viewport` | Capture game viewport |

### Console (4)

| Tool | Description |
|------|-------------|
| `clear_console` | Clear console output |
| `get_console_output` | Get console output |
| `get_console_errors` | Get console errors |
| `get_console_warnings` | Get console warnings |

### Debugger (15)

| Tool | Description |
|------|-------------|
| `get_debugger_state` | Get debugger state |
| `get_debugger_status` | Get debugger running status |
| `get_debugger_errors` | Get debugger error list |
| `get_fps` | Get current FPS |
| `get_memory_usage` | Get memory usage |
| `get_object_count` | Get object count |
| `get_performance_monitors` | Get performance monitor data |
| `get_physics_stats` | Get physics stats |
| `get_render_stats` | Get render stats |
| `get_stack_trace` | Get stack trace |
| `get_locals` | Get local variables |
| `debugger_break` | Pause execution (breakpoint) |
| `debugger_continue` | Continue execution |
| `debugger_control` | Debugger control (play/pause/step) |
| `debugger_step_into` | Step into |
| `debugger_step_out` | Step out |
| `debugger_step_over` | Step over |

### Breakpoints (3)

| Tool | Description |
|------|-------------|
| `list_breakpoints` | List all breakpoints |
| `set_breakpoint` | Set a breakpoint |
| `remove_breakpoint` | Remove a breakpoint |

### Workspace Switching (5)

| Tool | Description |
|------|-------------|
| `set_workspace` | Switch workspace (2D/3D/Script/AssetLib) |
| `set_workspace_2d` | Switch to 2D workspace |
| `set_workspace_3d` | Switch to 3D workspace |
| `set_workspace_script` | Switch to Script workspace |
| `set_workspace_assetlib` | Switch to Asset Library workspace |

## Runtime Bridge (6)

| Tool | Description |
|------|-------------|
| `get_game_scene_tree` | Get game runtime scene tree |
| `get_game_node_property` | Get game runtime node property |
| `set_game_node_property` | Set game runtime node property |
| `call_method_in_game` | Call method in game runtime |
| `capture_game_screenshot` | Capture game runtime screenshot |
| `simulate_game_input` | Simulate game runtime input |

## Runtime Lifecycle (6)

| Tool | Description |
|------|-------------|
| `run_project` | Run the project |
| `run_current_scene` | Run the current scene |
| `run_specific_scene` | Run a specific scene |
| `stop_project` | Stop running |
| `pause_project` | Pause/resume running |
| `set_movie_maker` | Toggle Movie Maker mode |
