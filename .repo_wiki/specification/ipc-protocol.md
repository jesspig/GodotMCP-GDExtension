# IPC 协议规范

> `godot-mcp-server`（Python） ↔ `godot_mcp_gdext.dll`（C++）之间的通信协议。

## 两条通信路径

### 路径 A: Legacy WebSocket IPC

- **协议**: WebSocket (`ws://127.0.0.1:9500`)
- **端口**: 9500（通过 `GODOT_MCP_PORT` 环境变量覆盖）
- **编码**: JSON，UTF-8
- **连接**: 接受多个连接，每个独立处理

### 路径 B: MCP Streamable HTTP

- **协议**: HTTP POST/GET/DELETE (`http://127.0.0.1:9600/mcp`)
- **端口**: 9600（通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖）
- **编码**: JSON-RPC 2.0 over HTTP，SSE for server-initiated events
- **协议版本**: MCP `"2025-11-25"` / `"2025-03-26"`

## 路径 A: WebSocket IPC 类型

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
| `code` | `String` | 错误码 |
| `message` | `String` | 错误描述 |

### IpcNotification（GDExt → Server）

连接就绪通知：
```json
{
    "type": "notification",
    "event": "godot_ready",
    "data": {
        "engine_version": "4.6.0",
        "plugin_version": "0.1.5-dev3"
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

## 路径 B: MCP Streamable HTTP 协议

### 会话建立

```
1. Client → POST /mcp {"method": "initialize", ...}    → 200 + MCP-Session-Id header
2. Client → GET /mcp (Accept: text/event-stream)       → 200 SSE stream
3. Client → POST /mcp {"method": "notifications/initialized"}  → 202
```

### 工具调用

```json
// Request: POST /mcp
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
        "name": "get_node_position",
        "arguments": {"node_path": "Player"}
    }
}

// Response: 200 application/json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "content": [{"type": "text", "text": "{\"x\": 100.0, \"y\": 200.0}"}]
    }
}
```

### MCP 支持的方法

| 方法 | 说明 |
|------|------|
| `initialize` | 创建会话，协商协议版本 |
| `notifications/initialized` | 确认初始化完成 |
| `ping` | 保活 |
| `tools/list` | 列出可用工具（分页，50/页） |
| `tools/call` | 执行工具 |
| `resources/list` | 空数组（不支持） |
| `prompts/list` | 空数组（不支持） |
| `logging/setLevel` | 设置日志级别 |

### SSE 事件

```
id: 1
event: message
data: {"jsonrpc":"2.0","method":"notifications/message","params":{...}}
```

## 协议流程

### 路径 A: WebSocket 启动

```
1. gdext 启动 WebSocket 服务器，监听 :9500
2. server 的 bridge.py 连接后 → 立即收到 godot_ready 通知
3. server 进入就绪状态，接受工具调用
```

### 路径 A: 工具调用

```
1. Server → GDExt: IpcRequest { method: "tool_call", params: { tool, args } }
2. GDExt HandlerRegistry::find() → 主线程同步执行 CommandFn
3. GDExt → Server: IpcResponse { status, data/error }
```

### 路径 B: HTTP 直接连接

```
1. AI 客户端 → POST /mcp (initialize) → 获取 MCP-Session-Id
2. AI 客户端 → GET /mcp (SSE) → 打开事件流
3. AI 客户端 → POST /mcp (tools/call) → 获取结果
```

## 错误处理

| 情况 | 行为 |
|------|------|
| 无效 JSON | Godot `JSON::parse` 失败，返回 `INVALID_REQUEST` |
| WebSocket 断开 | server 侧触发重连（指数退避，最多 5 次） |
| 无效 tool_call 参数 | 返回 `INVALID_REQUEST` |
| 未知工具 | `HandlerRegistry::find()` 返回 `UNKNOWN_TOOL` |
| HTTP Origin 无效 | 拒绝非 `127.0.0.1`/`localhost`/`null` 来源 |
| HTTP 会话无效 | 返回 400/401 |
