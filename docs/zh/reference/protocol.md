# MCP Streamable HTTP 协议

> AI 客户端通过 HTTP 直连 GDExtension 的通信协议。

## 传输方式

- **协议**: 纯 HTTP POST（`http://127.0.0.1:9600/mcp`）
- **端口**: 9600（通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖）
- **编码**: JSON-RPC 2.0 over HTTP，SSE for server-initiated events
- **协议版本**: MCP `"2026-07-28"`（无状态，无会话）

## 会话模型

MCP 2026-07-28 协议是**无状态**的。没有会话建立握手，没有 `MCP-Session-Id` 头，没有 `GET /mcp` 或 `DELETE /mcp` 端点。每个请求都是自包含的。

```
Client → POST /mcp (任意 JSON-RPC 2.0 消息) → 200 + JSON-RPC Response
```

## 工具调用

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

// Response: 200
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "content": [{"type": "text", "text": "{\"x\": 100.0, \"y\": 200.0}"}]
    }
}
```

## MCP 支持的方法

| 方法 | 说明 |
|------|------|
| `ping` | 保活 |
| `tools/list` | 列出可用工具 |
| `tools/call` | 执行工具 |
| `resources/list` | 空数组（不支持） |
| `prompts/list` | 空数组（不支持） |
| `logging/setLevel` | 设置日志级别 |
| `notifications/cancelled` | 取消正在执行的请求 |

## SSE 事件

SSE 事件内联在 POST 响应体中推送，用于流式结果：

```
id: 1
event: message
data: {"jsonrpc":"2.0","method":"notifications/message","params":{...}}
```

## 错误处理

| 情况 | 行为 |
|------|------|
| 无效 JSON | 返回 `INVALID_REQUEST` |
| 无效 tool_call 参数 | 返回 `INVALID_REQUEST` |
| 未知工具 | 返回 `UNKNOWN_TOOL` |
| HTTP Origin 无效 | 拒绝非 `127.0.0.1`/`localhost`/`null` 来源 |
