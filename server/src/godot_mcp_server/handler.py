import asyncio
import json
from pathlib import Path
from typing import Any

from godot_mcp_server.bridge import GodotBridge
from godot_mcp_server.editor_ctl import (
    SERVER_VERSION,
    godot_editor_close as _close_editor,
    godot_editor_open as _open_editor,
    kill_process_by_name,
    resolve_godot_path,
    resolve_project_path,
)
from godot_mcp_server.protocol import ToolListUpdate
from godot_mcp_server.registry import ToolRegistry

RECONNECT_MAX_ATTEMPTS = 5
RECONNECT_INITIAL_DELAY = 1.0
RECONNECT_MAX_DELAY = 30.0

_OFFLINE_MESSAGES: dict[str, str] = {
    "ping": "Godot 编辑器未连接",
    "get_engine_version": "Godot 编辑器未连接，无法获取引擎版本",
    "get_plugin_version": "Godot 编辑器未连接，无法获取插件版本",
}


class GodotMcpHandler:
    def __init__(
        self,
        port: int = 9500,
        godot_path_override: str | None = None,
    ) -> None:
        self._port = port
        self._godot_path_override = godot_path_override
        self.registry = ToolRegistry()
        self._bridge: GodotBridge | None = None

    async def handle_tool_call(
        self, name: str, args: dict[str, Any]
    ) -> str:
        if not self.registry.has_tool(name):
            return json.dumps({"error": f"Unknown tool: {name}"})
        if not self.registry.is_tool_enabled(name):
            return json.dumps({"error": f"Tool '{name}' is disabled"})

        if name == "get_server_version":
            return json.dumps({"version": SERVER_VERSION})

        if name == "godot_editor_open":
            try:
                godot_path = resolve_godot_path(self._godot_path_override)
                project_path = resolve_project_path(args)
                result = _open_editor(godot_path, project_path)
                return json.dumps(result)
            except RuntimeError as e:
                return json.dumps({"error": str(e)})

        if name == "godot_editor_close":
            try:
                godot_path = resolve_godot_path(self._godot_path_override)
                result = _close_editor(godot_path)
                return json.dumps(result)
            except RuntimeError as e:
                return json.dumps({"error": str(e)})

        if name == "godot_editor_restart":
            return await self._handle_restart(args)

        offline_msg = _OFFLINE_MESSAGES.get(name, "Godot 编辑器未连接")
        return await self._forward_tool_call(name, args, offline_msg)

    async def _handle_restart(self, args: dict) -> str:
        try:
            godot_path = resolve_godot_path(self._godot_path_override)
        except RuntimeError as e:
            return json.dumps({"error": str(e)})

        exe_name = Path(godot_path).name
        was_running = kill_process_by_name(exe_name)
        if was_running:
            await self._disconnect()
            await asyncio.sleep(0.5)

        project_path = resolve_project_path(args)
        try:
            result = _open_editor(godot_path, project_path)
        except RuntimeError as e:
            return json.dumps({"error": str(e)})

        return json.dumps({
            "status": "restarted",
            "pid": result["pid"],
            "was_running": was_running,
            "godot_path": godot_path,
            "project_path": project_path,
            "hint": "Editor is restarting. First tool call will auto-reconnect with retry.",
        })

    async def _forward_tool_call(
        self, name: str, args: dict, offline_msg: str
    ) -> str:
        bridge = await self._ensure_bridge()
        if bridge is None:
            return json.dumps({"error": offline_msg})
        try:
            result = await bridge.call(name, args)
            return json.dumps({"result": result})
        except Exception as e:
            await self._disconnect()
            return json.dumps({"error": f"Godot 通信失败: {e}"})

    async def _ensure_bridge(self) -> GodotBridge | None:
        if self._bridge is not None and self._bridge.is_connected:
            return self._bridge

        delay = RECONNECT_INITIAL_DELAY
        for attempt in range(RECONNECT_MAX_ATTEMPTS):
            if self._bridge is not None and self._bridge.is_connected:
                return self._bridge

            b = GodotBridge(
                port=self._port,
                on_tool_list_updated=self._on_tool_list_updated,
            )
            try:
                await b.connect()
                self._bridge = b
                return b
            except Exception as e:
                if attempt + 1 < RECONNECT_MAX_ATTEMPTS:
                    delay = min(delay * 2, RECONNECT_MAX_DELAY)
                    await asyncio.sleep(delay)
                else:
                    return None
        return None

    async def _disconnect(self) -> None:
        if self._bridge is not None:
            try:
                await self._bridge.close()
            except Exception:
                pass
            self._bridge = None

    def _on_tool_list_updated(self, update: ToolListUpdate) -> None:
        self.registry.update_from_notification(update)
