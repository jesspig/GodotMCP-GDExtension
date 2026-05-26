# cython: language_level=3
"""Cython --embed entry point for godot-mcp-server.

Compiled to a standalone .exe via:
    cython entry.pyx --embed -o entry.c
    <cc> entry.c -o godot-mcp-server.exe -I<include> -L<lib> -lpython3
"""

import os
import sys


def _setup_paths() -> None:
    """Configure Python search paths for embedded mode.

    The compiled exe needs to locate:
    1. Python standard library (PYTHONHOME)
    2. Third-party packages in .venv site-packages
    3. The godot_mcp_server package
    """
    exe_dir = os.path.dirname(os.path.abspath(sys.executable))

    # Step 1: Set PYTHONHOME if not already set
    # Try locating Python home relative to python314.dll (next to exe)
    pyhome_candidates = [
        os.environ.get("PYTHONHOME"),
        exe_dir,  # dll copied next to exe in build/
        os.path.join(exe_dir, "..", "..", "pythoncore-3.14-64"),
    ]
    for candidate in pyhome_candidates:
        if candidate and os.path.isdir(os.path.join(candidate, "Lib")):
            os.environ.setdefault("PYTHONHOME", candidate)
            break

    # Step 2: Add .venv site-packages to sys.path for third-party modules
    server_dir = os.path.join(exe_dir, "..", "server")
    if os.path.isdir(server_dir):
        venv_site = os.path.join(server_dir, ".venv", "Lib", "site-packages")
        if os.path.isdir(venv_site):
            sys.path.insert(0, os.path.abspath(venv_site))

        src_dir = os.path.join(server_dir, "src")
        if os.path.isdir(src_dir):
            sys.path.insert(0, os.path.abspath(src_dir))

    # Step 3: Also try finding site-packages relative to PYTHONHOME
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


def main() -> None:
    server = Server("godot-mcp-server")
    handler = GodotMcpHandler(port=9500)

    @server.list_tools()
    async def handle_list_tools():
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
    async def handle_call_tool(name: str, arguments: dict | None):
        text = await handler.handle_tool_call(name, arguments or {})
        return [TextContent(type="text", text=text)]

    async def _run():
        options = InitializationOptions(
            server_name="godot-mcp-server",
            server_version="0.1.4",
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
