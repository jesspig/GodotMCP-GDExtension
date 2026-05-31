# 自定义 MCP 客户端

> `mcp_client.py`——使用 `httpx.AsyncClient` 实现的 MCP Streamable HTTP 客户端。

## 为什么不用官方 MCP SDK

MCP SDK v1.27.2 的 `streamable_http_client` 使用 pydantic 校验 JSON-RPC 消息中的 `id` 字段，要求其为 `int | str`。GodotMCP 使用 Godot 的 `JSON::stringify()` 序列化结果，Godot 内部 `Variant` 将所有数字存储为 `double`，导致整数 `0` 被序列化为 `0.0`，触发 pydantic 校验错误。

**解决方案**：跳过 SDK，直接用 `httpx.AsyncClient` + 手动 JSON-RPC 消息构建。

## 核心类

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

模拟 MCP SDK 的 `TextContent` 对象，使现有阶段文件的 `_parse()` 函数无需改动即可工作：

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

### 辅助函数

| 函数 | 说明 |
|------|------|
| `create_mcp_client()` | 异步上下文管理器，初始化和清理会话 |
| `call_tool(session, name, args)` | 调用工具并解析结果为字典 |
| `list_tools(session)` | 返回 `set[str]` 工具名称集合 |
| `_parse_result(result)` | 将 `ToolResult.content` 解析为 Python 字典 |
