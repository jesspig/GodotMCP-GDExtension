"""Entry point for godot-mcp-server.

Compiled to a standalone .exe via Cython --embed:
    cython entry.py --embed -o entry.c
    <cc> entry.c -o godot-mcp-server.exe -I<include> -L<lib> -lpython3
"""

# cython: language_level=3
# cython: boundscheck=False
# cython: wraparound=False
# cython: nonecheck=False

import os
import sys
from typing import Optional, List, Dict, Any


def _setup_paths() -> None:
    exe_dir = os.path.dirname(os.path.abspath(sys.executable))

    pyhome_candidates: List[Optional[str]] = [
        os.environ.get("PYTHONHOME"),
        exe_dir,
    ]
    for candidate in pyhome_candidates:
        if candidate and os.path.isdir(os.path.join(candidate, "Lib")):
            os.environ.setdefault("PYTHONHOME", candidate)
            break

    # exe lives in build/ (or wherever CMake puts it); project root is one level up.
    project_dir = os.path.abspath(os.path.join(exe_dir, ".."))
    server_dir = os.path.join(project_dir, "server")

    # Add .venv site-packages (at project root, NOT inside server/).
    venv_site = os.path.join(project_dir, ".venv", "Lib", "site-packages")
    if os.path.isdir(venv_site):
        sys.path.insert(0, os.path.abspath(venv_site))

    # Add server/src so godot_mcp_server package is importable.
    src_dir = os.path.join(server_dir, "src")
    if os.path.isdir(src_dir):
        sys.path.insert(0, os.path.abspath(src_dir))

    pyhome = os.environ.get("PYTHONHOME")
    if pyhome:
        site_packages = os.path.join(pyhome, "Lib", "site-packages")
        if os.path.isdir(site_packages) and site_packages not in sys.path:
            sys.path.append(os.path.abspath(site_packages))


_setup_paths()

import asyncio

from mcp.server.lowlevel import Server, NotificationOptions
from mcp.server.models import InitializationOptions
import mcp.server.stdio
from mcp.types import TextContent, Tool

from godot_mcp_server.handler import GodotMcpHandler
from godot_mcp_server.editor_ctl import SERVER_VERSION


def main() -> None:
    server = Server("godot-mcp-server")
    handler = GodotMcpHandler(port=9500)

    @server.list_tools()
    async def handle_list_tools() -> List[Tool]:
        infos = handler.registry.get_all_tools()
        return [
            Tool(
                name=info.name,
                description=info.description,
                inputSchema=info.input_schema,
            )
            for info in infos
        ]

    @server.call_tool()
    async def handle_call_tool(name: str, arguments: Optional[Dict[str, Any]]):
        text = await handler.handle_tool_call(name, arguments or {})
        return [TextContent(type="text", text=text)]

    async def _run() -> None:
        options = InitializationOptions(
            server_name="godot-mcp-server",
            server_version=SERVER_VERSION,
            capabilities=server.get_capabilities(
                notification_options=NotificationOptions(),
                experimental_capabilities={},
            ),
        )
        async with mcp.server.stdio.stdio_server() as (read, write):
            await server.run(read, write, options)

    asyncio.run(_run())


if __name__ == "__main__":
    main()