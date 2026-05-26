import json
import os
from unittest.mock import AsyncMock, MagicMock, patch

import pytest

from godot_mcp_server.editor_ctl import SERVER_VERSION
from godot_mcp_server.handler import GodotMcpHandler


@pytest.fixture
def handler():
    return GodotMcpHandler(port=9999)


@pytest.mark.asyncio
async def test_get_server_version(handler):
    result = await handler.handle_tool_call("get_server_version", {})
    assert json.loads(result) == {"version": SERVER_VERSION}


@pytest.mark.asyncio
async def test_unknown_tool(handler):
    result = await handler.handle_tool_call("nonexistent_tool", {})
    assert "error" in json.loads(result)


@pytest.mark.asyncio
async def test_editor_open_no_godot_path(handler):
    with patch.dict(os.environ, {}, clear=True):
        result = await handler.handle_tool_call("godot_editor_open", {})
        data = json.loads(result)
        assert "error" in data
        assert "GODOT_PATH" in data["error"]


@pytest.mark.asyncio
async def test_editor_close_no_godot_path(handler):
    with patch.dict(os.environ, {}, clear=True):
        result = await handler.handle_tool_call("godot_editor_close", {})
        data = json.loads(result)
        assert "error" in data
        assert "GODOT_PATH" in data["error"]


@pytest.mark.asyncio
async def test_forward_tool_call_no_bridge(handler):
    handler._ensure_bridge = AsyncMock(return_value=None)
    result = await handler.handle_tool_call("ping", {})
    data = json.loads(result)
    assert "error" in data


@pytest.mark.asyncio
async def test_disabled_tool(handler):
    handler.registry.set_tool_enabled("ping", False)
    result = await handler.handle_tool_call("ping", {})
    data = json.loads(result)
    assert "error" in data
    assert "disabled" in data["error"]


@pytest.mark.asyncio
async def test_forward_tool_call_with_bridge(handler):
    mock_bridge = AsyncMock()
    mock_bridge.is_connected = True
    mock_bridge.call = AsyncMock(return_value={"message": "pong"})
    handler._bridge = mock_bridge

    result = await handler.handle_tool_call("ping", {})
    data = json.loads(result)
    assert data == {"result": {"message": "pong"}}
    mock_bridge.call.assert_called_once_with(
        "ping", {}
    )
