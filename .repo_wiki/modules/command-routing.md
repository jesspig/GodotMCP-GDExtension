# 命令路由

## 完整的调用链路

```
AI 客户端 call_tool("get_node_position", {"node_path": "Player"})
  │
  ▼
godot-mcp-server / handler.rs
  ├─ match "get_server_version" → 直接返回（不转发）
  ├─ match "godot_editor_*" → 服务器端处理（不转发到 gdext）
  ├─ else → forward_tool_call(name, args)
  │
  ▼ (WebSocket :9500)
godot_mcp_gdext / ws_server.rs → route_tool_call()
  │
  ├─ MetaCommands → can_handle("get_node_position") → 否
  ├─ NodeCommands → can_handle("get_node_position") → 否
  ├─ PropertyCommands → can_handle("get_node_position") → 是！
  │   │
  │   ▼
  │   dispatcher.submit(move || cmd_get_node_position(args)).await
  │   │
  │   ▼
  │   (主线程 process_frame 泵)
  │   cmd_get_node_position() → EditorInterface → Node API
  │   │
  │   ▼ (返回值)
  │   WebSocket IPC 响应
  │
  ▼
godot-mcp-server → JSON-RPC 响应
  │
  ▼
AI 客户端 {"result": {"x": 100, "y": 200}}
```

## 路由链（`ws_server.rs` `dispatch()`）

```rust
// 检查顺序（17 组，全部在 create_registry() 中注册）
MetaCommands(3)              // ping, get_engine_version, get_plugin_version
NodeCommands(21)             // get_scene_tree, get_node_path, create_node, ... + transforms, info, script vars
PropertyCommands(21)         // get/set position, rotation, scale, visible, modulate, z_index, text, ...
CollisionCommands(2)         // add_circle_collision, add_rectangle_collision
FindCommands(4)              // find_nodes_by_name/type/group/script
ScriptHelpersCommands(3)     // call_method, get_variable, set_variable
ProjectSettingsCommands(3)   // get/set_project_setting, set_main_scene
SceneCommands(16)            // create/delete/rename scene, open/close/save, ... + is_scene_dirty
ScriptGdCommands(5)          // create/read/edit/validate/list GDScript
ScriptCsCommands(6)          // create/read/edit/list C#, csharp_build, csharp_create_solution
SearchCommands(3)            // find_in_file, search_project, find_and_replace
UndoCommands(2)              // undo, redo
Property3dCommands(6)        // get/set position/rotation/scale 3D
EditorControlCommands(6)     // play_current_scene, play_main_scene, stop_scene, is_scene_playing, refresh, get_editor_info
ProjectSettingsExtCommands(10) // display, physics, rendering, project info, layer names
PluginManagementCommands(2)  // list_plugins, set_plugin_enabled
InputMapCommands(4)          // list/add/set/remove input actions
```

**注意**：
- `NodeCommands` 包含了原有的 NodeConvenience 工具（`set_node_transform_2d/3d`、`get_node_info`、`get_script_variables`）
- `SceneCommands` 包含了原有的 SceneInfo 工具（`is_scene_dirty`）
- `MetaCommands` 的 `get_server_version` 在 `handler.rs` 中服务器端直接处理，不会到达 gdext

## 服务器端 vs gdext 工具

| 处理位置 | 工具 |
|---------|------|
| handler.rs（服务器端） | `get_server_version`, `godot_editor_open`, `godot_editor_close`, `godot_editor_restart` |
| gdext route_tool_call（122 个） | 其余所有工具 |

## 新增工具清单

添加新工具需要修改以下位置：

### 服务器侧（`crates/server/`）

| 文件 | 修改 |
|------|------|
| `src/tool_registry.rs` | 在 `register_defaults()` 中添加工具的 JSON Schema |
| `src/handler.rs` | 如果是服务器端工具（`godot_editor_*`），添加处理分支 |
| 测试 | `tool_registry.rs` 的 `total == 125` 断言需要更新 |
| 测试 | `handler.rs` 的 `total == 125` 断言也需要更新 |

### gdext 侧（`crates/gdext/`）

| 文件 | 修改 |
|------|------|
| `src/commands/xx.rs` | 实现 `cmd_your_tool()` 函数 + 在 `TOOL_NAMES` 中添加工具名 |
| `src/commands/mod.rs` | 将 `YourToolHandler` 加入 `create_registry()` 的返回列表（若为新的组） |
| `src/ipc/ws_server.rs` | 不需要修改——`dispatch()` 自动遍历所有已注册的 handler |

### 现有组 vs 新组

- **已有组**：如果新工具属于已有命令组（如 `NodeCommands`），只需在组内添加 `cmd_*` 函数并在 `TOOL_NAMES` 中注册
- **新组**：如果创建新命令组，需要在 `commands/mod.rs` 的 `create_registry()` 中注册。`ws_server.rs` 的 `dispatch()` 使用通用循环，无需额外修改

## 测试

- `handler.rs` 断言 `total == 125`（服务器侧工具数）
- `tool_registry.rs` 断言 `total == 125`（注册表工具数）
- 离线测试（无 Godot）：不能测试真实工具调用，但可以测试 schema 注册、工具列表查询、错误处理

## 注意

- 新增工具后**两者计数必须同步更新**，否则测试会失败
- `create_registry()` 和 `route_tool_call()` 现在完全通过通用机制同步，不再需要手动维护两组路由列表
