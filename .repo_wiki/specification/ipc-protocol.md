# MCP Streamable HTTP 协议规范

> AI 客户端通过 HTTP 直连 `godot_mcp_gdext.dll`（C++）的通信协议。

## 传输方式

- **协议**: HTTP POST/GET/DELETE (`http://127.0.0.1:9600/mcp`)
- **端口**: 9600（通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖）
- **编码**: JSON-RPC 2.0 over HTTP，SSE for server-initiated events
- **协议版本**: MCP `"2025-11-25"` / `"2025-03-26"`

## 会话建立

```
1. Client → POST /mcp {"method": "initialize", ...}    → 200 + MCP-Session-Id header
2. Client → GET /mcp (Accept: text/event-stream)       → 200 SSE stream
3. Client → POST /mcp {"method": "notifications/initialized"}  → 202
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

// Response: 200 application/json
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
| `initialize` | 创建会话，协商协议版本 |
| `notifications/initialized` | 确认初始化完成 |
| `ping` | 保活 |
| `tools/list` | 列出可用工具（返回全部，无分页） |
| `tools/call` | 执行工具 |
| `resources/list` | 空数组（不支持） |
| `prompts/list` | 空数组（不支持） |
| `logging/setLevel` | 设置日志级别 |

## SSE 事件

```
id: 1
event: message
data: {"jsonrpc":"2.0","method":"notifications/message","params":{...}}
```

## 协议流程

```
1. AI 客户端 → POST /mcp (initialize) → 获取 MCP-Session-Id
2. AI 客户端 → GET /mcp (SSE) → 打开事件流
3. AI 客户端 → POST /mcp (tools/call) → 获取结果
```

## 错误处理

| 情况 | 行为 |
|------|------|
| 场景 | 行为 | 错误码 |
|------|------|--------|
| 无效 JSON | Godot `JSON::parse` 失败，返回 Parse Error | `kParseError` (-32700) |
| 无效 method 或缺少字段 | 返回 Invalid Request | `kInvalidRequest` (-32600) |
| 未知 JSON-RPC method | 返回 Method Not Found | `kMethodNotFound` (-32601) |
| tools/call 参数错误或未知工具 | 返回 Invalid Params | `kInvalidParams` (-32602) |
| 工具执行抛异常 | 捕获异常，返回 Internal Error | `kInternalError` (-32603) |
| HTTP Origin 无效 | 拒绝非 `127.0.0.1`/`localhost`/`null` 来源 | HTTP 403 |
| HTTP 会话无效 | 拒绝非 `127.0.0.1`/`localhost`/`null` 来源 | HTTP 400/401 |
