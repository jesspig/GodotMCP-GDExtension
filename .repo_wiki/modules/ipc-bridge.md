# MCP Streamable HTTP 传输

> AI 客户端通过 MCP Streamable HTTP 协议直连 `godot_mcp_gdext.dll`。

## 通信流程

```mermaid
sequenceDiagram
    participant Client as AI 客户端
    participant HTTP as HttpServer (C++)
    participant MCP as McpHandler (C++)
    participant REG as HandlerRegistry
    
    Client->>HTTP: POST /mcp (initialize)
    HTTP->>MCP: handle_initialize()
    MCP-->>HTTP: Session UUID + capabilities
    HTTP-->>Client: 200 + MCP-Session-Id header
    
    Client->>HTTP: GET /mcp (SSE stream, MCP-Session-Id)
    HTTP-->>Client: 200 text/event-stream
    
    Client->>HTTP: POST /mcp (notifications/initialized)
    HTTP-->>Client: 202 Accepted
    
    Note over Client,REG: 工具调用
    
    Client->>HTTP: POST /mcp (tools/call)
    HTTP->>MCP: handle_tools_call()
    MCP->>REG: find(tool) → CommandFn(args)
    REG-->>MCP: Dictionary result
    MCP-->>HTTP: MCP content array
    HTTP-->>Client: 200 application/json
```

## HTTP 端点（均位于 `/mcp`）

| 方法 | 用途 | 关键验证 |
|------|------|----------|
| `POST /mcp` | 发送 JSON-RPC 请求/通知 | `MCP-Protocol-Version`, `Content-Type`, `Accept`, `Origin` |
| `GET /mcp` | 打开 SSE 流 | `Accept: text/event-stream`, `MCP-Session-Id` |
| `DELETE /mcp` | 终止会话 | `MCP-Session-Id` |
| `OPTIONS /mcp` | CORS 预检 | 标准 CORS 头 |

## MCP 会话管理

- 会话通过 `initialize` 请求创建，UUID v4 标识
- 支持协议版本 `"2025-11-25"` 和 `"2025-03-26"`
- 每个 session 维护独立的 SSE 事件队列
- `tools/list` 支持分页（50 个/页）
- 30 秒空闲超时，最大 32 个并发连接

## SSE 事件推送

`McpHandler::enqueue_event()` → `Session::sse_event_queue` → `HttpServer::flush_sse()` 格式化为标准 SSE 帧：

```
id: <incrementing>
event: message
data: <json>
```

## 错误码

| 错误场景 | C++ 错误码 |
|----------|-----------|
| 无效 JSON | `INVALID_REQUEST` |
| 未知 method | `INVALID_REQUEST` |
| 未知工具 | `UNKNOWN_TOOL` |
| 执行失败 | `TOOL_FAILED` |
| 内部错误 | `INTERNAL_ERROR` |
