# IPC 协议规范

> `godot-mcp-server`（Python） ↔ `godot_mcp_gdext.dll`（Rust）之间通过 WebSocket 的通信协议。

## 传输

- **协议**: WebSocket (`ws://127.0.0.1:9500`)
- **端口**: 9500（gdext 侧硬编码，server 侧可通过 `--port` CLI 参数配置）
- **编码**: JSON，UTF-8
- **连接**: 接受多个连接，每个独立处理（但 server 侧通常只连一个）
- **心跳**: 30s 间隔 Ping，90s 超时断开

## 类型定义

所有 Rust 类型定义在 `crates/core/src/protocol.rs` 中。Python 侧对应的 Pydantic 模型在 `server/src/godot_mcp_server/protocol.py` 中。

### IpcRequest（Server → GDExt）

```json
{
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "method": "tool_call",
    "params": {
        "tool": "ping",
        "args": {}
    }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | `String` | UUID 请求标识符，用于匹配响应 |
| `method` | `String` | 请求类型：`"tool_call"`（标准）或直接使用工具名（兼容） |
| `params` | `Value` | 请求参数，`tool_call` 时包含 `tool` + `args` |

### IpcResponse（GDExt → Server）

成功响应：
```json
{
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "status": "ok",
    "data": {"x": 100, "y": 200}
}
```

错误响应：
```json
{
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "status": "error",
    "code": -1,
    "message": "Node not found: 'MissingNode'"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | `String` | 匹配请求的标识符 |
| `status` | `"ok"` / `"error"` | 响应标签（通过 `#[serde(tag)]` 实现） |
| `data` | `Value` | 成功时的结果数据 |
| `code` | `i32` | 错误码（-1 通用，-3 参数错误） |
| `message` | `String` | 错误描述 |

### IpcNotification（GDExt → Server）

连接就绪通知：
```json
{
    "type": "notification",
    "event": "godot_ready",
    "data": {
        "engine_version": "4.6.0",
        "plugin_version": "0.1.4",
        "protocol_version": "1.0"
    }
}
```

工具列表更新通知：
```json
{
    "type": "notification",
    "event": "tool_list_updated",
    "data": {
        "tools": [{"name": "ping", "enabled": true}]
    }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | `String` | 固定为 `"notification"` |
| `event` | `String` | 事件类型（`godot_ready`, `tool_list_updated`） |
| `data` | `Value` | 事件数据 |

## 协议流程

### 启动

```
1. gdext 启动 WebSocket 服务器，监听 :9500
2. server 的 bridge.py 连接后 → 立即收到 godot_ready 通知
3. server 进入就绪状态，接受工具调用
```

### 工具调用

```
1. Server → GDExt: IpcRequest { method: "tool_call", params: { tool, args } }
2. GDExt dispatch() 处理 → 通过 dispatcher 提交到主线程
3. GDExt → Server: IpcResponse { status, data/error }
```

### 通知推送

GDExt 通过 `broadcast::Sender` 通道推送通知；`ws_server.rs` 的 `handle_connection` 使用 `tokio::select!` 同时监听 WebSocket 消息和 broadcast 通道。

## 错误处理

| 情况 | 行为 |
|------|------|
| 无效 JSON | `serde_json` 解析失败，跳过当前消息 |
| WebSocket 断开 | `handle_connection` 退出；server 侧触发重连（指数退避，最多 5 次） |
| 心跳超时（90s 无活动） | 主动关闭连接 |
| 无效 tool_call 参数 | 返回 `IpcResult::Error { code: -3 }` |
| 未知工具 | dispatch() 返回 `Err("Unknown tool: ...")` → `IpcResult::Error { code: -1 }` |

## Godot 值转换

当工具参数或返回值包含 Godot 特定类型（Vector2、Color 等）时，这些值使用 `j2v`/`v2j` 函数转换。详见[场景命令模式](../modules/scene-commands.md)。