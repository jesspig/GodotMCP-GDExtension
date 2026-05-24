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
            CMDS["commands/ (*.rs)"]
            LSP["lsp/"]
            DOCK["dock/"]
        end
    end
    
    subgraph Server["godot-mcp-server.exe"]
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
- 不包含初始化逻辑——所有初始化在 `EditorPlugin` 中进行

### `editor_plugin.rs`

参见[编辑器插件](../modules/editor-plugin.md)页。关键点：

- `McpEditorPlugin::_enter_tree()`: 初始化静态变量、启动 tokio 运行时、创建 WebSocket 服务器、连接 `process_frame` 信号
- `McpEditorPlugin::_exit_tree()`: 关闭 WebSocket、停止 tokio 运行时、清理静态变量
- `McpEditorPlugin::_process()`: **故意留空**（避免 `bind_mut` 死锁）

## 目录映射

| 目录 | 内容 | 工具数 |
|------|------|--------|
| `commands/meta.rs` | ping, 版本查询等 | 4 |
| `commands/node.rs` | 节点 CRUD、场景树、组管理 | 17 |
| `commands/property.rs` | 2D 属性 get/set | 19 |
| `commands/property_3d.rs` | 3D 属性 get/set | 6 |
| `commands/scene.rs` | 场景文件、编辑器标签操作 | 15 |
| `commands/collision.rs` | 碰撞体添加 | 2 |
| `commands/find.rs` | 节点搜索 | 4 |
| `commands/script_helpers.rs` | call_method, get/set_variable | 3 |
| `commands/project_settings.rs` | 项目设置读写 | 3 |
| `commands/script_gd.rs` | GDScript 文件操作 | 5 |
| `commands/script_cs.rs` | C# 文件操作、Solution 生成 | 6 |
| `commands/search.rs` | 文件搜索与替换 | 3 |
| `commands/undo.rs` | 撤销/重做 | 2 |
| `commands/mod.rs` | CommandHandler trait + 共享工具函数 | - |
| `ipc/ws_server.rs` | WebSocket 服务器，路由 | - |
| `ipc/plugin_state.rs` | PluginState 静态变量 | - |
| `lsp/` | GDScript LSP 验证 | - |
| `dock/` | 编辑器右侧 Dock | - |

## 工具注册（与 server 侧的对比）

`commands/mod.rs` 中的 `create_registry()` 构建了第二个注册表（8 个命令组用于工具名发现）：

```
MetaCommands        → ["ping", "get_engine_version", "get_plugin_version", "get_server_version"]
NodeCommands        → 11 个节点操作 + 6 个辅助
PropertyCommands    → 19 个 2D 属性
Property3dCommands  → 6 个 3D 属性
CollisionCommands   → 2 个
FindCommands        → 4 个
ScriptGdCommands    → 5 个
ScriptCsCommands    → 6 个
```

**注意**：`create_registry()` 有 8 个组，而 `route_tool_call` 有 17 个处理分支 —— 后者包含 `SceneCommands`、`ScriptHelpersCommands`、`SearchCommands`、`UndoCommands` 和 `ProjectSettingsCommands`，它们在 `create_registry()` 中不存在。**两边必须保持同步**。

## `commands/mod.rs` 共享工具

| 函数 | 说明 |
|------|------|
| `j2v(v: &Value) -> Variant` | JSON → Godot Variant 转换（处理 Vector2/3/4、Color、Rect2、Quaternion、Resource） |
| `v2j(v: &Variant) -> Value` | Godot Variant → JSON 转换 |
| `resolve_node(root: &Node, path: &str) -> Option<Gd<Node>>` | 节点路径解析器 |
| `pipe(result: Value) -> Result<Value, String>` | `json!({"error":"..."})` → `Err` |
| `try_load<T: GodotClass>(path: &str) -> Result<Gd<T>, String>` | 安全资源加载 |
| `get_current_scene_root() -> Gd<Node>` | 获取当前编辑场景根节点 |
| `with_editor_interface(f)` | 获取 EditorInterface 的辅助宏 |
| `make_path(value: &str) -> String` | 路径规范化（添加 `res://` 前缀） |