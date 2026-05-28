# IPC 协议规范

> `godot-mcp-server`（Python） ↔ `godot_mcp_gdext.dll`（C++）之间通过 WebSocket 的通信协议。

## 传输

- **协议**: WebSocket (`ws://127.0.0.1:9500`)
- **端口**: 9500（gdext 侧硬编码，server 侧可通过 `--port` CLI 参数配置）
- **编码**: JSON，UTF-8
- **连接**: 接受多个连接，每个独立处理（但 server 侧通常只连一个）
## 类型定义

使用 Godot 原生 `Dictionary`/`Variant`/`String` 在 JSON 与内部类型之间转换。Python 侧对应的 Pydantic 模型在 `server/src/godot_mcp_server/protocol.py` 中。

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
    "status": "success",
    "data": {"x": 100, "y": 200}
}
```

错误响应：
```json
{
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "status": "error",
    "code": "TOOL_FAILED",
    "message": "Node not found: 'MissingNode'"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | `String` | 匹配请求的标识符 |
| `status` | `"success"` / `"error"` | 响应状态 |
| `data` | `Value` | 成功时的结果数据 |
| `code` | `String` | 错误码（`INVALID_REQUEST`, `UNKNOWN_TOOL`, `TOOL_FAILED`, `INTERNAL_ERROR`） |
| `message` | `String` | 错误描述 |

### IpcNotification（GDExt → Server）

连接就绪通知：
```json
{
    "type": "notification",
    "event": "godot_ready",
    "data": {
        "engine_version": "4.6.0",
        "plugin_version": "0.1.5-dev2",
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
2. GDExt HandlerRegistry::find() → 主线程同步执行 CommandFn
3. GDExt → Server: IpcResponse { status, data/error }
```

### 通知推送

GDExt 通过 `ws_server_.broadcast()` 向所有连接推送通知。

## 错误处理

| 情况 | 行为 |
|------|------|
| 无效 JSON | Godot `JSON::parse` 失败，返回 `INVALID_REQUEST` |
| WebSocket 断开 | server 侧触发重连（指数退避，最多 5 次） |
| 无效 tool_call 参数 | 返回 `INVALID_REQUEST` |
| 未知工具 | `HandlerRegistry::find()` 返回 `UNKNOWN_TOOL` |