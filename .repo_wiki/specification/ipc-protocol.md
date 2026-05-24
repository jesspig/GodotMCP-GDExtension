# IPC 协议规范

> `godot-mcp-server` ↔ `godot_mcp_gdext.dll` 之间通过 WebSocket 的通信协议。

## 传输

- **协议**: WebSocket (`ws://127.0.0.1:9500`)
- **端口**: 9500（可通过 `--port` CLI 参数配置）
- **编码**: JSON，UTF-8，每消息以 `\n` 分隔
- **连接**: 最多一个客户端——额外的连接尝试被拒绝

## 类型定义

所有类型定义在 `crates/core/src/protocol.rs` 中。

### IpcRequest（Server → GDExt）

```json
{
    "tool": "ping",
    "args": {"node_path": "Player"},
    "id": "req-001"
}
```

- `tool`: 字符串 —— 要调用的工具名称
- `args`: 可选对象 —— 工具参数
- `id`: 字符串 —— 请求标识符，用于匹配响应

### IpcResponse（GDExt → Server）

```json
{
    "id": "req-001",
    "result": {"x": 100, "y": 200}
}
```

- `id`: 字符串 —— 匹配请求的标识符
- `result`: 值 —— 工具调用的结果

错误响应示例：

```json
{
    "id": "req-001",
    "result": {"error": "Node not found: 'MissingNode'"}
}
```

### IpcNotification（GDExt → Server）

```json
{
    "method": "mcp_log_message",
    "params": {"level": "info", "tool": "ping", "message": "Ping received"}
}
```

- `method`: 字符串 —— 通知类型标识
- `params`: 对象 —— 通知数据

## 工具发现

服务器启动时通过 `rmcp` 框架向 AI 客户端**发布** `list_request_tools` 响应，不需要通过 IPC 请求 gdext。工具注册表在服务器端**静态声明**。

## 错误处理

### 协议级别

| 情况 | 行为 |
|------|------|
| 无效 JSON | 连接关闭 |
| WebSocket 断开 | server 端触发重连逻辑 |
| 超时 | 每个工具调用可在 `bridge.rs` 中设置超时 |
| 未知工具 | gdext 对未知工具名返回 `{"error": "unknown tool"}` |

### 应用级别

所有 `cmd_*` 函数在错误时返回 `json!({"error": "message"})`。`pipe()` 辅助函数将其转换为 `Result::Err`：

```rust
pipe(d.submit(move || {
    if something_wrong {
        return json!({"error": "something went wrong"});
    }
    json!({"status": "ok"})
}).await)
```

## 序列化

```rust
// 发送
let request = IpcRequest { tool, args, id };
let bytes = serde_json::to_vec(&request)?;
ws_sender.send(bytes).await?;

// 接收
let bytes = ws_receiver.recv().await?;
let response: IpcResponse = serde_json::from_slice(&bytes)?;
```

## Godot 值转换

当工具参数或返回值包含 Godot 特定类型（Vector2、Color 等）时，这些值使用 `j2v`/`v2j` 函数转换。详见[场景命令模式](../modules/scene-commands.md)。
