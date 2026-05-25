# cython: language_level=3
"""Cython --embed entry point for godot-mcp-server.

Compiled to a standalone .exe via:
    cython entry.pyx --embed -o entry.c
    <cc> entry.c -o godot-mcp-server.exe -I<include> -L<lib> -lpython3
"""

import asyncio
import json

from mcp.server.lowlevel import Server, NotificationOptions
from mcp.server.models import InitializationOptions
import mcp.server.stdio
from mcp.types import TextContent, Tool

from godot_mcp_server.handler import GodotMcpHandler


def main() -> None:
    server = Server("godot-mcp-server")
    handler = GodotMcpHandler(port=9500)

    @server.list_tools()
    async def handle_list_tools():
        return handler.registry.get_all_tools()

    @server.call_tool()
    async def handle_call_tool(name: str, arguments: dict | None):
        result = await handler.handle_tool_call(name, arguments or {})
        return [TextContent(type="text", text=json.dumps(result))]

    async def _run():
        options = InitializationOptions(
            server_name="godot-mcp-server",
            server_version="0.1.4",
            capabilities=server.get_capabilities(
                notification_options=NotificationOptions(),
            ),
        )
        async with mcp.server.stdio.stdio_server() as (read, write):
            await server.run(read, write, options)

    asyncio.run(_run())


if __name__ == "__main__":
    main()
