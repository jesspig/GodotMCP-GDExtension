from __future__ import annotations

import asyncio
import json
from unittest.mock import AsyncMock, MagicMock

import pytest
import websockets as ws_lib

from godot_mcp_server.bridge import GodotBridge
from godot_mcp_server.protocol import ToolListUpdate


@pytest.fixture
def mock_ws():
    m = AsyncMock(spec=ws_lib.ClientConnection)
    m.close_code = None
    m.send = AsyncMock()
    m.close = AsyncMock()
    return m


async def test_call_sends_correct_request(mock_ws):
    b = GodotBridge(port=9500)
    b._ws = mock_ws
    b._reader_task = asyncio.create_task(asyncio.sleep(0))

    response_task = asyncio.create_task(b.call("ping", {}))
    await asyncio.sleep(0.05)

    assert mock_ws.send.called
    sent_text = mock_ws.send.call_args[0][0]
    sent = json.loads(sent_text)
    assert sent["method"] == "tool_call"
    assert sent["params"]["tool"] == "ping"
    assert sent["params"]["args"] == {}
    rid = sent["id"]
    assert len(rid) == 36

    await b._handle_response({"id": rid, "status": "ok", "data": {"pong": True}})
    result = await response_task
    assert result == {"pong": True}

    await b.close()


async def test_call_handles_error(mock_ws):
    b = GodotBridge(port=9500)
    b._ws = mock_ws
    b._reader_task = asyncio.create_task(asyncio.sleep(0))

    response_task = asyncio.create_task(b.call("fail_tool", {}))
    await asyncio.sleep(0.05)

    sent_text = mock_ws.send.call_args[0][0]
    rid = json.loads(sent_text)["id"]

    await b._handle_response(
        {"id": rid, "status": "error", "code": -1, "message": "Something broke"}
    )

    with pytest.raises(RuntimeError, match="Something broke"):
        await response_task

    await b.close()


async def test_notification_tool_list_updated():
    callback = MagicMock()
    b = GodotBridge(port=9500, on_tool_list_updated=callback)

    b._handle_notification(
        {
            "type": "notification",
            "event": "tool_list_updated",
            "data": {
                "tools": [
                    {"name": "ping", "enabled": True},
                    {"name": "create_node", "enabled": False},
                ]
            },
        }
    )

    callback.assert_called_once()
    update: ToolListUpdate = callback.call_args[0][0]
    assert len(update.tools) == 2
    assert update.tools[0].name == "ping"
    assert update.tools[0].enabled is True
    assert update.tools[1].name == "create_node"
    assert update.tools[1].enabled is False


async def test_close_cancels_pending(mock_ws):
    b = GodotBridge(port=9500)
    b._ws = mock_ws
    b._reader_task = asyncio.create_task(asyncio.sleep(0))

    fut1 = asyncio.get_running_loop().create_future()
    fut2 = asyncio.get_running_loop().create_future()
    b._pending["a"] = fut1
    b._pending["b"] = fut2

    await b.close()

    assert fut1.cancelled()
    assert fut2.cancelled()
    assert b._pending == {}
    mock_ws.close.assert_called_once()


async def test_is_connected(mock_ws):
    b = GodotBridge(port=9500)
    assert not b.is_connected

    b._ws = mock_ws
    mock_ws.close_code = None
    assert b.is_connected

    mock_ws.close_code = 1000
    assert not b.is_connected
