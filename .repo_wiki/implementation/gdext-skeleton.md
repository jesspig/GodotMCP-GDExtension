# GDExtension 实现文档

## 入口：`lib.rs`

**文件**：`crates/gdext/src/lib.rs`

```rust
use godot::prelude::*;
use godot::init::InitLevel;

mod editor_plugin;
mod ipc;

struct GodotMcpExtension;

#[gdextension]
unsafe impl ExtensionLibrary for GodotMcpExtension {
    fn min_level() -> InitLevel {
        InitLevel::Editor
    }
}
```

**关键细节**：
- `InitLevel::Editor` 是必须的——`InitLevel::Scene` 将不起作用
- `mod editor_plugin` 和 `mod ipc` 被声明
- `commands/` 目录存在但**未被声明为模块**——无 `commands/mod.rs`，不在 `lib.rs` 模块树中

## EditorPlugin：`editor_plugin.rs`

**文件**：`crates/gdext/src/editor_plugin.rs`

```rust
#[derive(GodotClass)]
#[class(tool, base=EditorPlugin)]
pub struct McpEditorPlugin {
    #[base]
    base: Base<EditorPlugin>,
    runtime: Option<tokio::runtime::Runtime>,
    shutdown: Option<Arc<Notify>>,
}
```

### enter_tree 流程

1. 从 `Engine::singleton().get_version_info()` 读取引擎版本（`major.minor.patch`）
2. 读取插件版本（`env!("CARGO_PKG_VERSION")`）
3. 构建 tokio runtime：`new_multi_thread().worker_threads(2).enable_all()`
4. 创建 `PluginState { engine_version, plugin_version }`
5. 创建 `Arc<Notify>` shutdown 信号
6. 创建 `IpcWebSocketServer::new(9500, state, shutdown)`
7. 在 tokio runtime 中派生 `server.run()`
8. 存储 runtime 和 shutdown，打印 `"Plugin loaded!"`

### exit_tree 流程

1. 取出 shutdown → `notify_one()`
2. 休眠 200ms 等待 server 优雅关闭
3. 丢弃 runtime
4. 打印 `"Plugin unloaded!"`

**关键并发模型**：
- Godot 主线程 → 创建/销毁 plugin，不阻塞
- tokio 线程池（2 worker） → 运行 WS server，处理连接和请求
- `plugin_state.rs` 中的 `PluginState` 通过 `Arc` 共享，无锁（初始化后只读）

## IPC 模块：`ipc/`

### `mod.rs`

```rust
pub mod ws_server;
pub mod plugin_state;
```

### `plugin_state.rs`

```rust
#[derive(Debug, Clone)]
pub struct PluginState {
    pub engine_version: String,
    pub plugin_version: String,
}
```

初始化后不可变。由 `ws_server` 在请求处理中读取。

### `ws_server.rs`

**结构体**：

```rust
pub struct IpcWebSocketServer {
    port: u16,
    state: Arc<PluginState>,
    shutdown: Arc<Notify>,
}
```

**方法签名**：

```rust
impl IpcWebSocketServer {
    pub fn new(port: u16, state: Arc<PluginState>, shutdown: Arc<Notify>) -> Self;
    pub async fn run(&mut self) -> anyhow::Result<()>;
    async fn handle_connection(ws, state: Arc<PluginState>);
    fn handle_request(request: &IpcRequest, state: &PluginState) -> IpcResponse;
}
```

**run() 循环**：

```
loop {
    tokio::select! {
        accept_result => 接受连接, 派生 handle_connection 任务
        shutdown.notified() => break
    }
}
```

**handle_connection() 流程**：

1. 发送 `godot_ready` 通知（含 engine_version, plugin_version, protocol_version: "1.0"）
2. 拆分 read/write
3. read 循环：解析 `IpcRequest` → `handle_request()` → 发回 `IpcResponse`
4. 错误或断开 → 退出循环

**handle_request() — 硬编码匹配**：

```rust
fn handle_request(request: &IpcRequest, state: &PluginState) -> IpcResponse {
    match request.method.as_str() {
        "ping" => Success { data: { "message": "pong" } },
        "get_engine_version" => Success { data: { "engine_version": "..." } },
        "get_plugin_version" => Success { data: { "plugin_version": "..." } },
        _ => Error { code: -2, message: "Unknown method: ..." },
    }
}
```

**消息交换示例**：

```
→ {"id":"123","method":"ping","params":{}}
← {"id":"123","status":"ok","data":{"message":"pong"}}

→ {"id":"124","method":"get_engine_version","params":{}}
← {"id":"124","status":"ok","data":{"engine_version":"4.5.1"}}

→ {"id":"125","method":"unknown_tool","params":{}}
← {"id":"125","status":"error","code":-2,"message":"Unknown method: unknown_tool"}
```

## 配置

### GDExtension 配置

```ini
# addons/godot_mcp/godot_mcp.gdextension
[configuration]
entry_symbol = "gdext_rust_init"
compatibility_minimum = "4.6"
reloadable = true
```

### Plugin 配置

```ini
# addons/godot_mcp/plugin.cfg
[plugin]
name="Godot MCP"
script=""    # 有意留空——所有逻辑在原生 GDExtension 中
```

## 下一阶段计划

详见 [Dock UI 面板设计](../design/dock-ui.md) 和 [分阶段实施计划](../planning/phases.md)。

待实现：
1. Dock UI 面板（`dock/` 子模块）
2. 命令路由（`commands/` 子模块，`CommandHandler` trait）
3. 48 个工具（6 分组，每分组一个 commands 文件）
4. 心跳机制（10s ping/pong）
5. 连接管理（多客户端，广播通知）