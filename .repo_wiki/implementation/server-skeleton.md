# MCP Server 实现文档

## 入口：`main.rs`

**文件**：`crates/server/src/main.rs`

```rust
use clap::Parser;

#[derive(Parser, Debug)]
#[command(name = "godot-mcp-server")]
struct Cli {
    #[arg(long, default_value_t = 9500)]
    godot_port: u16,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();
    let handler = handler::GodotMcpHandler::new(cli.godot_port);
    let service = handler
        .serve((tokio::io::stdin(), tokio::io::stdout()))
        .await?;
    service.waiting().await?;
    Ok(())
}
```

**关键细节**：
- 当前**仅支持 stdio 传输**。无 `--transport` 选项，无 HTTP 模式。
- `transports/` 目录存在但**未被声明为模块**
- 无 `--help` 之外的 CLI 选项
- 无日志配置（`tracing-subscriber` 未依赖）

## MCP 处理器：`handler.rs`

**文件**：`crates/server/src/handler.rs`

### 结构体

```rust
#[derive(Clone)]
pub struct GodotMcpHandler {
    godot_port: u16,
    bridge: Arc<Mutex<Option<Arc<GodotBridge>>>>,
}
```

`bridge` 被包装为 `Arc<Mutex<Option<...>>>`——懒连接模式：
- 首次工具调用时 `ensure_bridge()` → `GodotBridge::connect(port)` → 存入 `Some`
- 连接失败时返回 `None`
- 通信错误时 `take()` 重置为 `None`

### ServerHandler 实现

```rust
impl ServerHandler for GodotMcpHandler {
    fn get_info(&self) -> ServerInfo;
    async fn list_tools(&self, ...) -> Result<ListToolsResult, ErrorData>;
    async fn call_tool(&self, ...) -> Result<CallToolResult, ErrorData>;
}
```

不实现 `list_resources` 或 `read_resource`——仅提供 tools 能力。

### get_info 返回内容

```
protocol_version: V_2025_03_26
capabilities:     tools
server_info:      Godot MCP {CARGO_PKG_VERSION}
instructions:     "Start Godot editor with the Godot MCP plugin installed..."
```

### 4 个工具

| 工具 | 描述 | 输入 schema | 实现方式 |
|------|------|------------|---------|
| `ping` | 检测连接状态 | `{}` | bridge → `"ping"` |
| `get_engine_version` | 获取引擎版本 | `{}` | bridge → `"get_engine_version"` |
| `get_plugin_version` | 获取插件版本 | `{}` | bridge → `"get_plugin_version"` |
| `get_server_version` | 获取 Server 版本 | `{}` | 本地 `CARGO_PKG_VERSION` |

所有工具使用空 schema（`{ "type": "object", "properties": {}, "required": [] }`），通过 `Self::empty_schema()` 生成。

离线时返回兜底文本而非错误（例：`"Godot 编辑器未连接"`）。

### 桥接生命周期

```rust
async fn bridge_text(&self, method: &str, offline_message: &str) -> String {
    let bridge = match self.ensure_bridge().await {
        Some(b) => b,
        None => return offline_message.to_string(),
    };
    match bridge.call(method, json!({})).await {
        Ok(result) => serde_json::to_string(&result).unwrap_or_else(|_| "{}".into()),
        Err(e) => {
            self.bridge.lock().await.take();  // ← 重置桥接
            format!("Godot 通信失败: {}", e)
        }
    }
}
```

**错误恢复机制**：
1. 桥接方法调用失败 → `bridge.take()` 清除连接
2. 下次工具调用 → `ensure_bridge()` 重新连接
3. 无显式重试逻辑，依赖 AWS 的「下次调用时重连」策略

## WebSocket 客户端：`bridge.rs`

**文件**：`crates/server/src/bridge.rs`

### 结构体

```rust
pub struct GodotBridge {
    writer: Arc<Mutex<SplitSink<...>>>,
    pending: Arc<DashMap<String, oneshot::Sender<Value>>>,
}
```

### 连接

```rust
pub async fn connect(port: u16) -> anyhow::Result<Self>
```

1. `connect_async("ws://127.0.0.1:{port}")`
2. 拆分 WebSocket 为 write/read
3. 派生读取任务：解析 `IpcResponse` → 按 UUID 从 `DashMap` 取出 `oneshot::Sender` → 发送 `Value`

### RPC 调用

```rust
pub async fn call(&self, method: &str, params: Value) -> anyhow::Result<Value>
```

1. `Uuid::new_v4()`
2. 构建 `IpcRequest { id, method, params }`
3. 创建 `oneshot::channel`，sender 存入 `pending` DashMap
4. 序列化 JSON 并写入 WebSocket
5. 等待 receiver → 返回结果

**并发特性**：
- `DashMap` 允许无锁并发插入/移除
- `oneshot` 通道确保每个请求只有一个响应
- 读者任务与调用者完全分离

## 与 GDExtension 的消息交换

```
Server (bridge.rs)                    GDExtension (ws_server.rs)
     │                                        │
     │── connect_async ──────────────────────→│
     │←── WebSocket 握手 ─────────────────────│
     │←── notification: godot_ready ──────────│
     │                                        │
     │── IpcRequest("ping") ─────────────────→│
     │←── IpcResponse(ok, "pong") ────────────│
     │                                        │
     │── IpcRequest("get_engine_version") ───→│
     │←── IpcResponse(ok, "4.5.1") ───────────│
```

**无心跳**：当前代码中无 ping/pong 机制。若连接静默断开，将在下次 `bridge.call()` 时检测到错误。

## 待实现

详见 [分阶段实施计划](../planning/phases.md) 和 [当前实现状态](current-status.md)。

| 项目 | 描述 |
|------|------|
| Streamable HTTP 传输 | axum :8900/mcp 端点 |
| 48 个工具 | 6 分类，每分类一个 handler 文件 |
| 工具热切换 | `tool_list_updated` 通知 → `rebuild_tools()` |
| 心跳 | 10s WebSocket ping/pong |
| 自动重连 | 指数退避 1s/5s/30s |
| 日志 | tracing-subscriber 日志配置 |
| `--transport` CLI | stdio / http / all 模式选择 |