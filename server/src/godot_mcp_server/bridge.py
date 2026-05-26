from __future__ import annotations

import asyncio
import json
import uuid
from typing import Any, Callable

import websockets

from godot_mcp_server.protocol import (
    IpcNotification,
    IpcRequest,
    IpcResponse,
    ToolListUpdate,
)


class GodotBridge:
    """WebSocket 客户端，连接到 Godot 编辑器的 GDExtension WebSocket 服务端。

    对应 Rust crates/server/src/bridge.rs 的 GodotBridge。
    """

    def __init__(
        self,
        port: int = 9500,
        on_tool_list_updated: Callable[[ToolListUpdate], None] | None = None,
    ) -> None:
        self._port = port
        self._on_tool_list_updated = on_tool_list_updated
        self._ws: websockets.ClientConnection | None = None
        self._pending: dict[str, asyncio.Future] = {}
        self._lock = asyncio.Lock()
        self._reader_task: asyncio.Task | None = None
        self._closed = False

    @property
    def is_connected(self) -> bool:
        return self._ws is not None and not self._ws.close_code

    async def connect(self) -> None:
        url = f"ws://127.0.0.1:{self._port}"
        self._ws = await websockets.connect(url, ping_interval=None)
        self._reader_task = asyncio.create_task(self._reader_loop())

    async def close(self) -> None:
        self._closed = True
        if self._reader_task is not None:
            self._reader_task.cancel()
            try:
                await self._reader_task
            except asyncio.CancelledError:
                pass
        if self._ws is not None:
            await self._ws.close()
            self._ws = None
        async with self._lock:
            for fut in self._pending.values():
                if not fut.done():
                    fut.cancel()
            self._pending.clear()

    async def call(self, tool: str, args: dict[str, Any]) -> dict[str, Any]:
        rid = str(uuid.uuid4())
        request = IpcRequest(id=rid, method="tool_call", params={"tool": tool, "args": args})
        fut = asyncio.get_running_loop().create_future()
        async with self._lock:
            self._pending[rid] = fut
        try:
            text = request.model_dump_json()
            await self._ws.send(text)
            result = await fut
            if isinstance(result, dict) and "error" in result:
                raise RuntimeError(result["error"])
            return result if isinstance(result, dict) else {}
        except Exception:
            async with self._lock:
                self._pending.pop(rid, None)
            raise

    async def _reader_loop(self) -> None:
        try:
            async for raw in self._ws:
                if self._closed:
                    break
                try:
                    data = json.loads(raw)
                except json.JSONDecodeError:
                    continue
                if "type" in data and data["type"] == "notification":
                    self._handle_notification(data)
                elif "id" in data:
                    await self._handle_response(data)
        except websockets.ConnectionClosed:
            pass
        except asyncio.CancelledError:
            pass

    async def _handle_response(self, data: dict) -> None:
        rid = data.get("id")
        if rid is None:
            return
        async with self._lock:
            fut = self._pending.pop(rid, None)
        if fut is None or fut.done():
            return
        if data.get("status") == "error":
            fut.set_result({"error": data.get("message", "Unknown error")})
        else:
            fut.set_result(data.get("data", {}))

    def _handle_notification(self, data: dict) -> None:
        event = data.get("event", "")
        if event == "tool_list_updated" and self._on_tool_list_updated is not None:
            try:
                update = ToolListUpdate.model_validate(data.get("data", {}))
                self._on_tool_list_updated(update)
            except Exception:
                pass
