# 命令路由

## 完整的调用链路

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

## 路由实现对比

### C++（当前）—— HandlerRegistry

```cpp
// handler_registry.cpp
void handle_packet(...) {
    const CommandFn *fn = registry_->find(tool);
    if (!fn) {
        // 返回 "Unknown tool" 错误
        return;
    }
    Dictionary data = (*fn)(args);  // 同步执行，主线程
    // 检查 error 字段并返回响应
}
```

- `CommandFn = std::function<Dictionary(const Dictionary &)>`（函数指针）
- `HandlerRegistry` 本质上是 `HashMap<String, CommandFn>`
- `register_all_tools()` 调用 17 个 `register_<group>()` 自由函数来填充注册表

### Rust（遗留）—— CommandHandler trait + dispatch()

```rust
// ws_server.rs
async fn dispatch(tool, args, state, dispatcher, registry) {
    let meta = MetaCommands::new().with_engine_version(state.engine_version.clone());
    if meta.can_handle(tool) {
        return meta.handle_meta_tool(tool);  // 同步，无需 dispatcher
    }
    for handler in registry {
        if handler.can_handle(tool) {
            return handler.handle(tool, args, dispatcher).await;  // 通过 dispatcher
        }
    }
    Err(format!("Unknown tool: {}", tool))
}
```

- `CommandHandler` trait：`can_handle()` + `handle()` + `group_name()` + `tool_names()`
- MetaCommands（3 个工具）在 dispatch 外部单独处理（同步，无需 `MainThreadDispatcher`）
- 其余 17 组通过 `dispatcher.submit()` 提交到主线程

## 16 组 C++ 处理器

| # | 文件 | 工具数 | 工具前缀 |
|---|------|--------|----------|
| 1 | `meta.cpp` | 3 | ping, get_engine/plugin_version |
| 2 | `node.cpp` | 17 | create/delete/rename/duplicate/move_node, attach/detach_script, set_as_root, reset_parent, batch_set_property, add/remove_node_from_group, get_node_path |
| 3 | `property.cpp` | 19 | get/set_node_position/rotation/scale/visible/modulate/z_index/text/collision_layer/mask, set_node_unique_name |
| 4 | `property_3d.cpp` | 6 | get/set_node_position/rotation/scale_3d |
| 5 | `collision.cpp` | 2 | add_circle/rectangle_collision |
| 6 | `find.cpp` | 4 | find_nodes_by_name/type/group/script |
| 7 | `scene.cpp` | 15 | create/delete/rename_scene, branch_to_scene, scene_to_branch, instantiate_scene, open/close/save/save_as/save_all/reload_scene, get_open_scenes/roots, mark_scene_unsaved |
| 8 | `script_gd.cpp` | 5 | create/read/edit/list_gdscript, validate_gdscript |
| 9 | `script_cs.cpp` | 6 | create/read/edit/list_csharp_script, csharp_build, csharp_create_solution |
| 10 | `script_helpers.cpp` | 3 | call_method, get/set_variable |
| 11 | `project_settings.cpp` | 7 | get/set_project_setting, set_main_scene, list/remove/add_autoload, list_scenes |
| 12 | `project_settings_ext.cpp` | 10 | get/set_display/physics/rendering_settings, get/set_project_info, get/set_layer_names |
| 13 | `editor_control.cpp` | 6 | play_current/main_scene, stop_scene, is_scene_playing, refresh_filesystem, get_editor_info |
| 14 | `input_map.cpp` | 4 | list/add/remove_input_action, set_input_action_events |
| 15 | `plugin_management.cpp` | 2 | list_plugins, set_plugin_enabled |
| 16 | `undo.cpp` | 2 | undo, redo |
| 17 | `search.cpp` | 3 | find_in_file, search_project, find_and_replace |

## 注意事项

- `get_server_version`、`godot_editor_open/close/restart` 在 Python 服务器端处理，不转发到 gdext
- C++ 版本：所有命令在主线程同步执行，godot-cpp API 直接调用无限制
- Rust 版本：所有命令通过 `MainThreadDispatcher` 提交到主线程（MetaCommands 除外）
- 添加新工具时，`ws_server.cpp` **不需要修改**——`HandlerRegistry::find()` 自动覆盖注册的工具
