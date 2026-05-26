# 编辑器插件（`McpEditorPlugin`）

> `godot_mcp_gdext.dll` 的生命周期管理。

## 生命周期

```mermaid
stateDiagram-v2
    [*] --> EnterTree: Godot 加载插件
    
    EnterTree --> Initializing: enter_tree()
    
    state Initializing {
        [*] --> ReadVersion: 读取引擎版本
        ReadVersion --> CreateRuntime: 创建 tokio 运行时 (2 workers)
        CreateRuntime --> CreateState: Arc&lt;PluginState&gt;
        CreateRuntime --> CreateDispatcher: MainThreadDispatcher
        CreateRuntime --> CreateRegistry: create_registry() 17组 handler
        CreateDispatcher --> StartWS: IpcWebSocketServer::new(9500)
        StartWS --> SpawnServer: runtime.spawn(server.run())
        SpawnServer --> InstallPump: process_frame 信号安装
        InstallPump --> AddDock: add_control_to_dock RIGHT_UL
        AddDock --> [*]
    }
    
    Initializing --> Ready: 加载完成
    
    Ready --> Processing: 编辑器正常运行
    
    state Processing {
        [*] --> PumpDispatcher: 每帧泵 dispatcher
        PumpDispatcher --> PumpLogs: 泵日志队列
        PumpLogs --> [*]: 等待下一帧
    }
    
    Processing --> Exiting: 编辑器卸载 / exit_tree()
    
    state Exiting {
        [*] --> UninstallPump: 断开 process_frame 信号
        UninstallPump --> RemoveDock: remove_control_from_docks
        RemoveDock --> SendShutdown: shutdown.notify_one()
        SendShutdown --> WaitServer: sleep 200ms
        WaitServer --> CleanRuntime: drop runtime
        CleanRuntime --> [*]
    }
    
    Exiting --> [*]
```

## `enter_tree()` 初始化

```rust
fn enter_tree(&mut self) {
    let engine_version = Self::read_engine_version();
    let plugin_version = env!("CARGO_PKG_VERSION");

    // 1. 创建 tokio 运行时（2 worker threads）
    let runtime = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(2).enable_all().build();

    // 2. PluginState 为 Arc（非静态变量）
    let state = Arc::new(PluginState { engine_version, plugin_version });

    // 3. 创建 dispatcher 和 WebSocket 服务器
    let dispatcher = MainThreadDispatcher::new();
    let shutdown = Arc::new(Notify::new());
    let registry = commands::create_registry();
    let server = IpcWebSocketServer::new(9500, state, shutdown, dispatcher, registry);

    // 4. 在 tokio 上启动 WebSocket 服务器
    runtime.spawn(async move { server.run().await });

    // 5. 安装 process_frame 泵
    install_main_thread_pump(dispatcher);

    // 6. 添加 Dock UI
    add_control_to_dock(RIGHT_UL, &dock);
}
```

## `exit_tree()` 清理

```rust
fn exit_tree(&mut self) {
    // 1. 断开 process_frame 信号
    uninstall_main_thread_pump();

    // 2. 移除 Dock UI
    remove_control_from_docks(&dock);
    dock.free();

    // 3. 发送 WebSocket 服务器关闭信号
    shutdown.notify_one();
    std::thread::sleep(Duration::from_millis(200));

    // 4. 清理其他字段
    dispatcher = None;
    runtime.take();
    logging::drain_to_console();
}
```

## 为什么不用 `EditorPlugin::_process()`（bind_mut 陷阱）

`process_frame` 信号连接在 `SceneTree` 上，**不持有** `McpEditorPlugin` 的任何绑定：

```rust
let callable = Callable::from_fn("godot_mcp_pump", move |_args| {
    dispatcher.process_pending();
    logging::drain_to_console();
    Variant::nil()
});
tree.connect("process_frame", &callable);
```

这样调用栈上不会产生 `bind_mut` 死锁。**不要**将此逻辑移回 `_process()`。

**实现细节**：
- `install_main_thread_pump()`：在 `enter_tree()` 中连接信号
- `uninstall_main_thread_pump()`：在 `exit_tree()` 中断开信号
- 两个函数都使用 Option 字段（`pump_callable`, `scene_tree`）来跟踪状态

## `PluginState`

`PluginState` 是简单的 Arc 共享结构体（不再是通过 `OnceLock` 访问的全局静态变量）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `engine_version` | `String` | Godot 引擎版本号（如 "4.6.0"） |
| `plugin_version` | `String` | MCP 插件版本号（来自 CARGO_PKG_VERSION） |

传递给 `IpcWebSocketServer`，用于 MetaCommands 的 `get_engine_version`。

## Broadcast 通道

`McpEditorPlugin` 持有 `broadcast::Sender<String>`，用于向所有连接的 WebSocket 客户端推送通知：

```rust
pub fn broadcast_tool_list_updated(&self, tools: Value) {
    let notification = IpcNotification {
        msg_type: "notification".into(),
        event: "tool_list_updated".into(),
        data: tools,
    };
    tx.send(serde_json::to_string(&notification).unwrap());
}
```