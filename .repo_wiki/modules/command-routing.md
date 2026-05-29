# 命令路由

## 完整的调用链路

### 路径 A: Legacy stdio → WebSocket

```
AI 客户端 call_tool("get_node_position", {"node_path": "Player"})
  │
  ▼
godot-mcp-server (Python) / handler.py
  ├─ match "get_server_version" → 直接返回（不转发）
  ├─ match "godot_editor_*" → 服务器端处理（不转发到 gdext）
  ├─ else → _forward_tool_call(name, args)
  │
  ▼ (WebSocket :9500)
godot_mcp_gdext / ws_server.cpp → handle_packet()
  │
  ├─ 解析 JSON → 验证 method=="tool_call" → 提取 tool + args
  ├─ HandlerRegistry::find(tool) → 查找 CommandFn
  │   ├─ 未找到 → 返回 "Unknown tool: ..." 错误
  │   └─ 找到 → 执行 CommandFn(args) ← 主线程，同步
  │       │
  │       ▼
  │       函数调用 EditorInterface → Node API
  │       │
  │       ▼ (返回值 Dictionary)
  │       if data.has("error") → 返回错误响应
  │       else → 返回成功响应 {"status":"success", "data": {...}}
  │
  ▼
godot-mcp-server → JSON-RPC 响应（TextContent）
  │
  ▼
AI 客户端 {"result": {"x": 100, "y": 200}}
```

### 路径 B: MCP Streamable HTTP

```
AI 客户端 HTTP POST /mcp {"method": "tools/call", "params": {"name": "get_node_position", "arguments": {...}}}
  │
  ▼
godot_mcp_gdext / http_server.cpp → handle_post()
  │
  ├─ 验证 MCP-Protocol-Version / Content-Type / Accept / Origin
  ├─ 解析 JSON-RPC 2.0 消息
  │
  ▼
mcp_handler.cpp → handle_tools_call()
  │
  ├─ 检查 server-only 工具 → 返回空结果（由 stdio 服务器处理）
  ├─ HandlerRegistry::find(tool) → 查找 CommandFn
  │   └─ 执行 CommandFn(args) ← 主线程，同步
  │       │
  │       ▼ (返回值 Dictionary)
  │       包装为 MCP content array: [{"type":"text","text":...}]
  │
  ▼
HTTP 200 application/json + MCP-Session-Id header
  │
  ▼
AI 客户端
```

## HandlerRegistry 实现

- `CommandFn = std::function<Dictionary(const Dictionary &)>`（函数指针）
- `HandlerRegistry` 本质上是 `HashMap<String, CommandFn>`
- `register_all_tools()` 调用 16 个 `register_<group>()` 自由函数来填充注册表
- 另有 1 个 `register_<group>()` 已声明但未调用（`register_script_cs`）

## 17 个 C++ 处理器文件

| # | 文件 | register_ 函数 | 调用状态 | 工具数 | 工具前缀 |
|---|------|---------------|---------|--------|----------|
| 1 | `meta.cpp` | `register_meta` | **活跃** | 3 | ping, get_engine/plugin_version |
| 2 | `node.cpp` | `register_node` | **活跃** | 21 | get_scene_tree, get_node_path, get_property_list, get_property, create/delete/rename/duplicate/move_node, set_property, attach/detach_script, set_as_root, reset_parent, batch_set_property, add/remove_node_from_group, set_node_transform_2d/3d, get_node_info, get_script_variables |
| 3 | `property.cpp` | `register_property` | **活跃** | 21 | get/set_node_position/rotation/scale/visible/modulate/z_index/text/collision_layer/mask/texture, set_node_unique_name |
| 4 | `property_3d.cpp` | `register_property_3d` | **活跃** | 6 | get/set_node_position/rotation/scale_3d |
| 5 | `collision.cpp` | `register_collision` | **活跃** | 2 | add_circle/rectangle_collision |
| 6 | `find.cpp` | `register_find` | **活跃** | 4 | find_nodes_by_name/type/group/script |
| 7 | `scene.cpp` | `register_scene` | **活跃** | 16 | create/delete/rename_scene, branch_to_scene, scene_to_branch, instantiate_scene, open/close/save/save_as/save_all/reload_scene, get_open_scenes/roots, mark_scene_unsaved, is_scene_dirty |
| 8 | `script_gd.cpp` | `register_script_gd` | **活跃** | 5 | create/read/edit/list_gdscript, validate_gdscript |
| 9 | `script_cs.cpp` | `register_script_cs` | **未调用** | 6 | create/read/edit/list_csharp_script, csharp_build, csharp_create_solution |
| 10 | `script_helpers.cpp` | `register_script_helpers` | **活跃** | 3 | call_method, get/set_variable |
| 11 | `project_settings.cpp` | `register_project_settings` | **活跃** | 7 | get/set_project_setting, set_main_scene, list/add/remove_autoload, list_scenes |
| 12 | `project_settings_ext.cpp` | `register_project_settings_ext` | **活跃** | 10 | get/set_display/physics/rendering_settings, get/set_project_info, get/set_layer_names |
| 13 | `editor_control.cpp` | `register_editor_control` | **活跃** | 6 | play_current/main_scene, stop_scene, is_scene_playing, refresh_filesystem, get_editor_info |
| 14 | `input_map.cpp` | `register_input_map` | **活跃** | 4 | list/add/remove_input_action, set_input_action_events |
| 15 | `plugin_management.cpp` | `register_plugin_management` | **活跃** | 2 | list_plugins, set_plugin_enabled |
| 16 | `undo.cpp` | `register_undo` | **活跃** | 2 | undo, redo |
| 17 | `search.cpp` | `register_search` | **活跃** | 3 | find_in_file, search_project, find_and_replace |

**总计**：17 个文件定义 121 个工具，16 组在 `register_all_tools()` 中活跃注册（115 个），`register_script_cs`（6 个 C# 工具）已声明但未调用。

Python 侧 `registry.py` 注册全部 125 个工具 schema（含 4 个服务器端 + 121 个 gdext），不区分 gdext 侧是否活跃。

## 注意事项

- `get_server_version`、`godot_editor_open/close/restart` 在 Python 服务器端处理，不转发到 gdext
- 所有命令在主线程同步执行，godot-cpp API 直接调用无限制
- 添加新工具时，`ws_server.cpp` 和 `http_server.cpp` **都不需要修改**——`HandlerRegistry::find()` 自动覆盖注册的工具
