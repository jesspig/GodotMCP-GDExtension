# 自定义 MCP 客户端（已废弃——历史参考）

> **状态：已被简化方案替代。** `tests/mcp_client.py` 文件已删除。当前编排器 `test_orchestrator.py` 通过 `/run-tests` HTTP 端点（由 C++ `TestEngine` 解析 YAML）执行测试，不再走 MCP 客户端。
>
> **保留此文档仅作为历史参考**。

## 为什么不用官方 MCP SDK

MCP SDK v1.27.2 的 `streamable_http_client` 使用 pydantic 校验 JSON-RPC 消息中的 `id` 字段，要求其为 `int | str`。GodotMCP 使用 Godot 的 `JSON::stringify()` 序列化结果，Godot 内部 `Variant` 将所有数字存储为 `double`，导致整数 `0` 被序列化为 `0.0`，触发 pydantic 校验错误。

**解决方案**：跳过 SDK，直接用 `httpx.AsyncClient` + 手动 JSON-RPC 消息构建。

## 旧核心类

### `McpSession`

```python
class McpSession:
    def __init__(self, base_url: str = "http://127.0.0.1:9600/mcp")
    async def __aenter__()  # 创建 httpx 客户端
    async def initialize()  # POST /mcp (initialize) → 获取 Session-ID
    async def list_tools()  # POST /mcp (tools/list)
    async def call_tool(name, args)  # POST /mcp (tools/call)
    async def ping()
    async def close()  # DELETE /mcp
```

- 内部维护 `_session_id`（通过 `Mcp-Session-Id` 响应头提取）
- `_request()` 方法处理：JSON-RPC 2.0 封装、会话 ID 注入、错误码检查、多响应展开（数组→取第一个）
- 每次 `call_tool` 递增 JSON-RPC `id` 计数器

### `_TextContent`

模拟 MCP SDK 的 `TextContent` 对象：

```python
class _TextContent:
    type: str = "text"
    text: str  # 自动 JSON.dumps(list/dict)
    annotations: Optional[dict] = None
```

### `ToolResult`

```python
class ToolResult:
    content: list[_TextContent]
    isError: bool = False
```

## 当前编排器路径

`test_orchestrator.py` 不再使用 `McpSession`：

| 阶段 | 旧做法 | 当前做法 |
|------|--------|----------|
| 工具发现 | `await session.list_tools()` | `GET /run-tests?action=list`（返回 YAML 文件列表） |
| 工具验证 | `await session.call_tool(name, args)` | `POST /run-tests`（body = YAML 文本） |
| 结果解析 | `result.content[0].text` → JSON 解析 | `/run-tests` 直接返回 JSON 报告 |

详见 [testing/orchestrator.md](orchestrator.md)。
