# `crates/gdext` — GDExtension CDyLib

> 加载到 Godot 编辑器内的本机插件。包含 125 个工具命令的实现。

```mermaid
flowchart LR
    subgraph Godot["Godot Editor Process"]
        subgraph DLL["godot_mcp_gdext.dll"]
            EP["editor_plugin.rs<br/>McpEditorPlugin"]
            IP["ipc/ws_server.rs<br/>IpcWebSocketServer"]
            DISP["dispatcher.rs<br/>MainThreadDispatcher"]
            LOG["logging.rs"]
            CMDS["commands/ (17个模块)"]
            LSP["lsp/"]
            DOCK["dock/"]
        end
    end
    
    subgraph Server["godot-mcp-server.exe (Python)"]
        WS["WebSocket Client"]
    end
    
    Server <-->|tool_call IPC| IP
    IP --> DISP
    DISP --> CMDS
    CMDS -.->|Godot API| EP
    LOG -.->|process_frame pump| EP
    EP -.->|Godot signals| DISP
    EP -.->|Godot signals| LOG
    
    DOCK ---|EditorInterface.get_editor_main_screen().add_control_to_dock| EP
    LSP ---|EditorInterface| EP
```

## 文件

### `lib.rs`

- `#[gdextension]` 入口
- `InitLevel::Editor` — 仅在编辑器模式下加载（不加载到运行游戏）
- `unsafe impl ExtensionLibrary` — Godot 4.6+ 的 Rust 绑定模式
- `mcp/` 目录存在但为空（预留）

### `editor_plugin.rs`

参见[编辑器插件](../modules/editor-plugin.md)页。关键点：

- `McpEditorPlugin::enter_tree()`: 创建 tokio 运行时、启动 WebSocket 服务器、安装主线程泵、添加 Dock UI
- `McpEditorPlugin::exit_tree()`: 发送 shutdown 信号、清理泵、移除 Dock、停止运行时
- `McpEditorPlugin::broadcast_tool_list_updated()`: 通过 broadcast 通道向所有 WebSocket 客户端推送工具列表更新
- `_process()`: **未重写**——GodotClass 默认无帧更新逻辑

## 目录映射

| 目录/文件 | 内容 | 工具数 |
|-----------|------|--------|
| `commands/meta.rs` | ping, get_engine_version, get_plugin_version | 3 |
| `commands/node.rs` | 节点 CRUD、场景树、组管理、transform | 17 |
| `commands/property.rs` | 2D 属性 get/set | 19 |
| `commands/property_3d.rs` | 3D 属性 get/set | 6 |
| `commands/scene.rs` | 场景文件、编辑器标签操作、autoload | 15 |
| `commands/collision.rs` | 碰撞体添加 | 2 |
| `commands/find.rs` | 节点搜索 | 4 |
| `commands/script_helpers.rs` | call_method, get/set_variable | 3 |
| `commands/project_settings.rs` | 项目设置读写、autoload、list_scenes | 7 |
| `commands/script_gd.rs` | GDScript 文件操作 | 5 |
| `commands/script_cs.rs` | C# 文件操作、Solution 生成、build | 6 |
| `commands/search.rs` | 文件搜索与替换 | 3 |
| `commands/undo.rs` | 撤销/重做 | 2 |
| `commands/editor_control.rs` | play_scene, stop, refresh, get_editor_info | 6 |
| `commands/project_settings_ext.rs` | 显示/物理/渲染/层名设置 | 10 |
| `commands/plugin_management.rs` | 列出/启用/禁用插件 | 2 |
| `commands/input_map.rs` | 输入动作管理 | 4 |
| `commands/mod.rs` | CommandHandler trait + 共享工具函数 | - |
| `ipc/ws_server.rs` | WebSocket 服务器，dispatch() 路由 | - |
| `ipc/plugin_state.rs` | PluginState 结构体（非静态） | - |
| `lsp/` | GDScript LSP 验证 | - |
| `dock/` | 编辑器右侧 Dock | - |

**总计: 121 个 gdext 工具 + 4 个服务器端工具 = 125**

## 工具注册

`commands/mod.rs` 中的 `create_registry()` 构建 17 个 CommandHandler 组：

```
create_registry():
  0. MetaCommands          → ["ping", "get_engine_version", "get_plugin_version"]
  1. NodeCommands          → 17 个节点操作
  2. PropertyCommands      → 19 个 2D 属性
  3. CollisionCommands     → 2 个
  4. FindCommands          → 4 个
  5. ScriptHelpersCommands → 3 个
  6. ProjectSettingsCmds   → 7 个
  7. SceneCommands         → 15 个
  8. ScriptGdCommands      → 5 个
  9. ScriptCsCommands      → 6 个
  10. SearchCommands       → 3 个
  11. UndoCommands         → 2 个
  12. Property3dCommands   → 6 个
  13. EditorControlCmds    → 6 个
  14. ProjectSettingsExt   → 10 个
  15. PluginManagement     → 2 个
  16. InputMapCommands     → 4 个
```

`ws_server.rs::dispatch()` 先独立处理 MetaCommands（同步，无需 MainThreadDispatcher），再迭代 registry。

## `commands/mod.rs` 共享工具

| 函数 | 说明 |
|------|------|
| `s(args, key) -> String` | 从 JSON args 读取字符串字段（默认空串） |
| `pipe(val) -> Result<Value, String>` | 展平 dispatcher 结果 + 检查 JSON 级 error |
| `ensure_parent_dir(path)` | 创建 `res://` 路径的父目录（**主线程调用**） |
| `resolve_node(root, path) -> Option<Gd<Node>>` | 节点路径解析器 |
| `v2j(v) -> Value` | Variant → JSON（Vector2/3/4, Color, Rect2, Quaternion, Resource） |
| `j2v(v) -> Variant` | JSON → Variant（自动 `try_load` res:///user:// 路径） |
| `globalize_path(res_path) -> String` | res:// 路径 → OS 绝对路径（**主线程调用**） |
| `sn(s) -> StringName` | &str → StringName 辅助 |
| `relative_path(node, root) -> String` | 编辑器内部路径 → 场景相对路径（如 "Pong/Ball"） |
| `get_root() -> Result<Gd<Node>, Value>` | 获取当前编辑场景根节点 |
| `get_undo_redo() -> Gd<EditorUndoRedoManager>` | 获取编辑器 UndoRedo |
| `undoable_set(node, prop, value, name)` | 可撤销属性设置 |
| `mark_dirty()` | 标记当前场景为未保存 |
| `fix_owners_recursive(node, owner)` | 递归修复节点 owner |
| `node_replace_owner(base, old, new, ur, mode)` | 通过 UndoRedo 替换 owner |