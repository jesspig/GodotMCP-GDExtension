import asyncio
import json
from contextlib import asynccontextmanager

import httpx


class McpSession:
    """Minimal MCP Streamable HTTP client for GodotMCP testing."""

    def __init__(self, client: httpx.AsyncClient, url: str, session_id: str | None = None):
        self._client = client
        self._url = url
        self._session_id = session_id
        self._request_id = 0
        self._server_info = None
        self.server_capabilities = {}

    async def initialize(self):
        return await self._request("initialize", {
            "protocolVersion": "2025-03-26",
            "capabilities": {},
            "clientInfo": {"name": "godot-mcp-test", "version": "1.0.0"},
        })

    async def list_tools(self):
        resp = await self._request("tools/list", {})
        return [ToolInfo(**t) for t in resp.get("tools", [])]

    async def call_tool(self, name: str, arguments: dict | None = None):
        return self._make_result(await self._request("tools/call", {
            "name": name,
            "arguments": arguments or {},
        }))

    async def ping(self):
        return await self._request("ping", {})

    def _make_result(self, resp: dict) -> "ToolResult":
        content_raw = resp.get("content", [])
        return ToolResult(content=content_raw, isError=resp.get("isError", False))

    async def _request(self, method: str, params: dict) -> dict:
        self._request_id += 1
        body = {
            "jsonrpc": "2.0",
            "id": self._request_id,
            "method": method,
            "params": params,
        }
        headers = {"Content-Type": "application/json", "Accept": "application/json"}
        if self._session_id:
            headers["MCP-Session-Id"] = self._session_id

        for attempt in range(2):
            resp = await self._client.post(self._url, json=body, headers=headers)
            if resp.status_code == 200:
                break
            if attempt == 0 and resp.status_code in (400, 405):
                await asyncio.sleep(0.1)
                continue
            raise McpError({"code": resp.status_code, "message": f"HTTP {resp.status_code}: {resp.text[:200]}"})

        # Extract session ID from response headers
        sid = resp.headers.get("mcp-session-id")
        if sid:
            self._session_id = sid

        try:
            data = resp.json()
        except Exception:
            raise McpError({"code": resp.status_code, "message": f"HTTP {resp.status_code}: {resp.text[:200]}"})

        if isinstance(data, list):
            data = data[0] if data else {}

        if "error" in data:
            raise McpError(data["error"])

        # Handle both JSON-RPC envelope and flat result
        if "result" in data:
            result = data["result"]
        else:
            result = data

        if method == "initialize":
            self._server_info = result.get("serverInfo", {})
            self.server_capabilities = result.get("capabilities", {})

        return result


class ToolInfo:
    def __init__(self, name: str, description: str = "", inputSchema: dict | None = None):
        self.name = name
        self.description = description
        self.inputSchema = inputSchema or {}


class _TextContent:
    """Mimics MCP SDK TextContent so phase _parse() functions work unchanged."""
    def __init__(self, d: dict):
        self.type = d.get("type", "text")
        self.text = d.get("text", d.get("_raw", str(d)))
        if isinstance(self.text, (list, dict)):
            import json as _json
            self.text = _json.dumps(self.text)


class ToolResult:
    def __init__(self, content: list, isError: bool = False):
        self.content = [_TextContent(c) if isinstance(c, dict) else c for c in content]
        self.isError = isError


class McpError(Exception):
    def __init__(self, error: dict):
        self.code = error.get("code")
        self.message = error.get("message", "MCP Error")
        self.data = error.get("data")
        super().__init__(self.message)


@asynccontextmanager
async def create_mcp_client(url: str = "http://127.0.0.1:9600/mcp"):
    limits = httpx.Limits(max_keepalive_connections=0, max_connections=1)
    async with httpx.AsyncClient(timeout=30, limits=limits) as client:
        session = McpSession(client, url)
        await session.initialize()
        yield session


async def call_tool(session: McpSession, tool_name: str, arguments: dict | None = None):
    result = await session.call_tool(tool_name, arguments)
    return _parse_result(result)


async def list_tools(session: McpSession) -> set[str]:
    tools = await session.list_tools()
    return {t.name for t in tools}


def _parse_result(result: ToolResult):
    content = result.content if hasattr(result, "content") else []
    texts = []
    for c in content:
        if isinstance(c, dict):
            texts.append(c.get("text", str(c)))
        else:
            texts.append(str(c))
    combined = "\n".join(texts)
    if not combined:
        return {}
    if combined.startswith("{"):
        try:
            return json.loads(combined)
        except json.JSONDecodeError:
            pass
    if combined.startswith("[") and combined.endswith("]"):
        try:
            return json.loads(combined)
        except json.JSONDecodeError:
            pass
    return {"_raw": combined}
