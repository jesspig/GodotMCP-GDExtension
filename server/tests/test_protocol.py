import json

from godot_mcp_server.protocol import (
    IpcNotification,
    IpcRequest,
    IpcResponse,
    ToolCallParams,
    ToolInfo,
    ToolListUpdate,
    ToolState,
)


def test_ipc_request_roundtrip():
    req = IpcRequest(id="test-uuid", method="ping", params={})
    d = req.model_dump()
    assert d == {"id": "test-uuid", "method": "ping", "params": {}}
    restored = IpcRequest.model_validate(d)
    assert restored.id == "test-uuid"
    assert restored.method == "ping"
    assert restored.params == {}


def test_ipc_response_success():
    resp = IpcResponse.ok(id="abc", data={"message": "pong"})
    d = resp.model_dump(mode="json")
    assert d["status"] == "ok"
    assert d["data"]["message"] == "pong"
    assert d["id"] == "abc"

    raw = json.dumps(d)
    restored = IpcResponse.model_validate_json(raw)
    assert restored.id == "abc"
    assert restored.status == "ok"
    assert restored.data["message"] == "pong"


def test_ipc_response_error():
    resp = IpcResponse.error(id="xyz", code=-2, message="Unknown method")
    d = resp.model_dump(mode="json")
    assert d["status"] == "error"
    assert d["code"] == -2
    assert d["message"] == "Unknown method"

    raw = json.dumps(d)
    restored = IpcResponse.model_validate_json(raw)
    assert restored.id == "xyz"
    assert restored.status == "error"
    assert restored.code == -2
    assert restored.message == "Unknown method"


def test_ipc_response_deserialize_success():
    raw = r'{"id":"1","status":"ok","data":{"value":42}}'
    resp = IpcResponse.model_validate_json(raw)
    assert resp.id == "1"
    assert resp.status == "ok"
    assert resp.data["value"] == 42


def test_ipc_response_deserialize_error():
    raw = r'{"id":"2","status":"error","code":-1,"message":"fail"}'
    resp = IpcResponse.model_validate_json(raw)
    assert resp.id == "2"
    assert resp.status == "error"
    assert resp.code == -1
    assert resp.message == "fail"


def test_ipc_notification_roundtrip():
    notif = IpcNotification(event="godot_ready", data={"engine_version": "4.5.1"})
    d = notif.model_dump(by_alias=True, mode="json")
    assert d["type"] == "notification"
    assert d["event"] == "godot_ready"
    assert d["data"]["engine_version"] == "4.5.1"
    assert "msg_type" not in d

    raw = json.dumps(d)
    restored = IpcNotification.model_validate_json(raw)
    assert restored.msg_type == "notification"
    assert restored.event == "godot_ready"
    assert restored.data["engine_version"] == "4.5.1"


def test_ipc_request_empty_params():
    raw = r'{"id":"a","method":"ping","params":{}}'
    req = IpcRequest.model_validate_json(raw)
    assert req.params == {}


def test_ipc_request_nested_params():
    raw = r'{"id":"b","method":"tool_invoke","params":{"tool":"create_node","args":{"x":1}}}'
    req = IpcRequest.model_validate_json(raw)
    assert req.params["tool"] == "create_node"
    assert req.params["args"]["x"] == 1


def test_tool_call_params_roundtrip():
    params = ToolCallParams(
        tool="create_node",
        args={"parent_path": "/root", "node_type": "Node2D", "name": "Player"},
    )
    d = params.model_dump(mode="json")
    assert d["tool"] == "create_node"
    assert d["args"]["parent_path"] == "/root"

    restored = ToolCallParams.model_validate(d)
    assert restored.tool == "create_node"
    assert restored.args["parent_path"] == "/root"


def test_tool_call_params_empty_args():
    params = ToolCallParams(tool="ping")
    assert params.args == {}

    raw = json.dumps(params.model_dump(mode="json"))
    restored = ToolCallParams.model_validate_json(raw)
    assert restored.tool == "ping"
    assert restored.args == {}


def test_tool_call_params_default_args():
    raw = r'{"tool":"get_scene_tree"}'
    params = ToolCallParams.model_validate_json(raw)
    assert params.tool == "get_scene_tree"
    assert params.args == {}


def test_tool_info_default_enabled():
    assert ToolInfo(name="ping", description="test", input_schema={}).enabled is True


def test_tool_info_explicit_disabled():
    info = ToolInfo(name="debug_tool", description="test", input_schema={}, enabled=False)
    assert info.enabled is False


def test_tool_list_update_roundtrip():
    update = ToolListUpdate(
        tools=[
            ToolState(name="ping", enabled=True),
            ToolState(name="debug_draw", enabled=False),
        ]
    )
    d = update.model_dump(mode="json")
    assert len(d["tools"]) == 2
    assert d["tools"][0]["enabled"] is True
    assert d["tools"][1]["enabled"] is False

    restored = ToolListUpdate.model_validate(d)
    assert restored.tools[0].enabled is True
    assert restored.tools[1].enabled is False


def test_tool_state_json_format():
    raw = r'{"tools":[{"name":"ping","enabled":true},{"name":"stop","enabled":false}]}'
    update = ToolListUpdate.model_validate_json(raw)
    assert update.tools[0].name == "ping"
    assert update.tools[1].name == "stop"


def test_ipc_notification_json_wire():
    """Verify the exact JSON format sent over WebSocket matches Rust's serde output."""
    notif = IpcNotification(event="godot_ready", data={"engine_version": "4.5.1"})
    raw = notif.model_dump_json(by_alias=True)
    parsed = json.loads(raw)
    assert parsed == {"type": "notification", "event": "godot_ready", "data": {"engine_version": "4.5.1"}}
    assert "msg_type" not in parsed


def test_ipc_response_success_wire():
    """Verify the exact JSON format matches Rust's serde output."""
    resp = IpcResponse.ok(id="abc", data={"message": "pong"})
    raw = resp.model_dump_json()
    parsed = json.loads(raw)
    assert parsed == {"id": "abc", "status": "ok", "data": {"message": "pong"}, "code": 0, "message": ""}
