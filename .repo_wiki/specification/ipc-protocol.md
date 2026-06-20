# MCP Streamable HTTP 协议规范

> AI 客户端通过 HTTP 直连 `godot_mcp_gdext.dll`（C++）的通信协议。实现 MCP Streamable HTTP（无 session），支持 2026-07-28 / 2025-11-25 / 2025-03-26。

## 传输方式

- **方法**: `POST` / `OPTIONS` 仅限（无 `GET`，无 `DELETE`）
- **端点**: `http://127.0.0.1:9600/mcp`
- **端口**: 9600（`GODOT_MCP_HTTP_PORT` 环境变量覆盖）
- **编码**: JSON-RPC 2.0 over HTTP
- **协议版本**: `2026-07-28` / `2025-11-25` / `2025-03-26`（兜底 `2025-03-26`）
- **CORS**: OPTIONS 预检返回 `Access-Control-Allow-Methods: POST, OPTIONS`；暴露 `MCP-Protocol-Version`、`Last-Event-ID`
- **速率限制**: 30 req/s（全局），超限 HTTP 429
- **认证**: 可选 Bearer token（`Authorization: Bearer <token>`），常量时间比较

## 会话模型

**无 session，无 handshake。** 每请求独立处理。`server/discover` 取代 MCP 2025 的 `initialize`，返回 capabilities + serverInfo。

## HTTP 请求头

| 请求头 | 必填 | 说明 |
|--------|------|------|
| `Content-Type` | 是 | `application/json` |
| `Accept` | 是 | `application/json` 或 `*/*` |
| `MCP-Protocol-Version` | 是 | 客户端期望的协议版本，服务端兜底为 `2025-03-26` |
| `Mcp-Method` | 推荐 | 与 body 的 `method` 字段一致，不一致返回 `HeaderMismatch` 错误 |
| `Mcp-Name` | 推荐 | 与 body 的 `params.name` 或 `params.uri` 一致，不一致返回 `HeaderMismatch` 错误 |
| `Authorization` | 条件 | 服务端配置 token 时必须 |

**`Mcp-Method` / `Mcp-Name` 双重校验**：即使 body 解析正确，header 与 body 不匹配也拒绝请求。这防止 AI 客户端幻觉发送错误 method 或工具名。

## 请求 / 响应

### 工具调用

```
POST /mcp
Content-Type: application/json
Accept: application/json
MCP-Protocol-Version: 2026-07-28
Mcp-Method: tools/call
Mcp-Name: get_node_position
```

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
        "name": "get_node_position",
        "arguments": {"node_path": "Player"}
    }
}
```

成功响应:

```
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
```

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "content": [{"type": "text", "text": "{\"x\": 100.0, \"y\": 200.0}"}]
    }
}
```

### 通知（无响应）

通知类消息（`notifications/cancelled`）返回 HTTP 202，无响应 body:

```
POST /mcp
Content-Type: application/json
MCP-Protocol-Version: 2026-07-28
Mcp-Method: notifications/cancelled
```

```json
{
    "jsonrpc": "2.0",
    "method": "notifications/cancelled",
    "params": {"requestId": "1"}
}
```

```
HTTP/1.1 202 Accepted
```

### services/discover（取代 initialize）

```
POST /mcp
Content-Type: application/json
MCP-Protocol-Version: 2026-07-28
Mcp-Method: server/discover
```

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "server/discover",
    "params": {}
}
```

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "capabilities": {
            "tools": {"listChanged": true},
            "resources": {},
            "prompts": {},
            "logging": {},
            "completions": {}
        },
        "serverInfo": {
            "name": "godot-mcp",
            "version": "0.2.2-dev1"
        }
    }
}
```

### 批量请求

请求体为 JSON 数组时触发批量模式。每个元素独立处理，返回与请求等长的结果数组。仅带通知的 batch 返回 202。

```
POST /mcp
Content-Type: application/json
MCP-Protocol-Version: 2026-07-28
```

```json
[
    {"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": "1"}},
    {"jsonrpc": "2.0", "id": 2, "method": "ping"}
]
```

## 支持的方法

| 方法 | 分类 | 说明 |
|------|------|------|
| `server/discover` | Lifecycle | 取代 `initialize`，返回 capabilities + serverInfo |
| `ping` | Lifecycle | 保活，返回空 result |
| `tools/list` | Tools | 列出 always-on 工具（渐进式披露） |
| `tools/call` | Tools | 执行工具 |
| `resources/list` | Resources | 列出 godot:// 资源（scene-tree/project-settings/editor-info） |
| `resources/read` | Resources | 读取指定资源内容 |
| `resources/templates/list` | Resources | 列出 URI 模板（如 `scene-node/{path}`） |
| `prompts/list` | Prompts | 列出内置提示模板 |
| `prompts/get` | Prompts | 获取提示内容 |
| `completion/complete` | Utilities | 自动补全（资源 URI / prompt 参数） |
| `notifications/cancelled` | Notifications | 客户端取消请求（无响应） |

## SSE 事件

支持两种 SSE 交付方式，无需客户端轮询：

### GET /mcp 长连接

`GET /mcp`（需 `Accept: text/event-stream`）建立长期 SSE 连接，服务端在有事件时通过同一连接推送：

```
GET /mcp
Accept: text/event-stream

→ 200 text/event-stream
→ retry: 5000
→ event: endpoint
→ data: {"endpoint":"/mcp"}
```

连接保持后，事件按需推送。每 15s 发送 keepalive 注释行防止代理超时断开。

### POST 内联响应

当 POST 请求处理前已有 pending 事件时，响应以 `Content-Type: text/event-stream` 返回，事件发送完毕后关闭连接。

### SSE 格式

```
event: message
data: {"jsonrpc":"2.0","method":"notifications/tools/list_changed","params":{}}
```

### 完整响应示例（内联 SSE）

```
HTTP/1.1 200 OK
Content-Type: text/event-stream; charset=utf-8
Cache-Control: no-cache
Connection: keep-alive
X-Accel-Buffering: no

retry: 5000
id: 1
event: message
data: {"jsonrpc":"2.0","method":"notifications/tools/list_changed","params":{}}
```

### 通知事件类型

| 事件 | 说明 |
|------|------|
| `notifications/tools/list_changed` | 工具列表已变更，客户端应重新拉取 `tools/list` |

### 触发条件

SSE 内联发生时机：服务端在处理当前请求前已有 pending 事件（如前一 tick 中工具注册变更），或当前请求产生新事件。事件写入完成后 TCP 连接关闭。客户端无需长连接保持。

## 错误处理

| 场景 | 行为 | HTTP 状态 | 错误码 |
|------|------|-----------|--------|
| 无效 JSON（parse 失败） | 返回 Parse Error | 200 | `kParseError` (-32700) |
| 无效 method 或缺少字段 | 返回 Invalid Request | 200 | `kInvalidRequest` (-32600) |
| Mcp-Method / Mcp-Name header 与 body 不一致 | Header 校验拒绝 | 400 | -32600（HeaderMismatch） |
| 未知 JSON-RPC method | 返回 Method Not Found | 200 | `kMethodNotFound` (-32601) |
| `tools/call` 缺少 `name` 参数 | 返回 Invalid Params | 200 | `kInvalidParams` (-32602) |
| `resources/read` 参数无效 | 返回 Invalid Params | 200 | `kInvalidParams` (-32602) |
| 未知工具名 | 工具执行失败 → Internal Error | 200 | `kInternalError` (-32603) |
| 工具执行抛异常 | 捕获异常 → Internal Error | 200 | `kInternalError` (-32603) |
| 资源 URI 不存在 | 返回 Resource Not Found | 200 | `kResourceNotFound` (-32002) |
| 请求被取消 | 请求已被 notifications/cancelled | 200 | `kServerTerminated` (-32001) |
| 请求体为空 | 拒绝 | 400 | — |
| Content-Type 非 JSON | 拒绝 | 415 | — |
| Accept 不含 JSON | 拒绝 | 406 | — |
| Origin 无效 | 非 `127.0.0.1`/`localhost`/`null` 来源 | 403 | — |
| 速率超限 | 拒绝 | 429 | — |
| 认证失败 | 拒绝 | 401 | — |
| HTTP 方法非 POST/OPTIONS | 拒绝 | 405 | — |
| 路径非 `/mcp` 或 `/run-tests` | 拒绝 | 404 | — |

JSON-RPC 错误响应示例：

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "error": {
        "code": -32601,
        "message": "Unknown method: tools/delete"
    }
}
```

HeaderMismatch 错误响应（HTTP 400）：

```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -32600,
        "message": "HeaderMismatch: Mcp-Method does not match body method"
    },
    "id": null
}
```

## 协议版本协商

服务端在 `MCP-Protocol-Version` 请求头中读取客户端版本：

1. 如果版本在 `{2026-07-28, 2025-11-25, 2025-03-26}` 中 → 直接返回该版本
2. 如果版本为空或不支持 → 兜底为 `2025-03-26`
3. 实际协商发生在 `server/discover` 响应的 `protocolVersion` 字段

**响应头始终包含 `MCP-Protocol-Version`**，指示实际使用的协议版本。

## 验证

```bash
# Ping
curl -s -X POST http://127.0.0.1:9600/mcp \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -d '{"jsonrpc":"2.0","id":1,"method":"ping"}'

# server/discover
curl -s -X POST http://127.0.0.1:9600/mcp \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: server/discover" \
  -d '{"jsonrpc":"2.0","id":1,"method":"server/discover","params":{}}'

# tools/list
curl -s -X POST http://127.0.0.1:9600/mcp \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/list" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}'

# tools/call
curl -s -X POST http://127.0.0.1:9600/mcp \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/call" \
  -H "Mcp-Name: ping" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"ping"}}'

# Header mismatch test（预期 400）
curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:9600/mcp \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/call" \
  -d '{"jsonrpc":"2.0","id":1,"method":"ping"}'
# → 400

# Unsupported method（预期 405）
curl -s -o /dev/null -w "%{http_code}" -X GET http://127.0.0.1:9600/mcp
# → 405

# Bad path（预期 404）
curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:9600/foo
# → 404
```
