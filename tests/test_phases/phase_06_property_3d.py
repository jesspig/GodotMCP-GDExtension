import json

from .base import ToolTest, TestResult
from file_verifier import get_node_property

TEST_SCENE = "res://test_mcp_prop3d/test_3d_scene.tscn"


async def _setup_3d(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Node3D",
        "root_name": "Root3D",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": TEST_SCENE})
    await ctx.session.call_tool("create_node", {
        "name": "TestNode3D",
        "node_type": "Node3D",
        "parent_path": ".",
    })
    ctx.temp_scene_path = TEST_SCENE
    ctx.created_nodes.append("TestNode3D")


async def _teardown_3d(ctx):
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene", {"path": TEST_SCENE})
    except Exception:
        pass


async def test_get_node_position_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_position_3d", {"node_path": "TestNode3D"})
    parsed = _parse(data)
    return TestResult("get_node_position_3d", "PASS" if parsed else "FAIL",
                      expected="position {x,y,z}", actual=parsed)


async def test_set_node_position_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_position_3d", {
        "node_path": "TestNode3D", "x": 10, "y": 20, "z": 30,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_pos = get_node_property(TEST_SCENE, "TestNode3D", "position")

    data2 = await ctx.session.call_tool("get_node_position_3d", {"node_path": "TestNode3D"})
    parsed2 = _parse(data2)
    return TestResult("set_node_position_3d", "PASS",
                      expected="x=10,y=20,z=30", actual=parsed2,
                      disk_verified=disk_pos is not None,
                      disk_detail=f"disk position={disk_pos}")


async def test_get_node_rotation_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_rotation_3d", {"node_path": "TestNode3D"})
    parsed = _parse(data)
    return TestResult("get_node_rotation_3d", "PASS" if parsed else "FAIL",
                      expected="rotation {x,y,z}", actual=parsed)


async def test_set_node_rotation_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_rotation_3d", {
        "node_path": "TestNode3D", "rot_x": 15, "rot_y": 30, "rot_z": 45,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_rot = get_node_property(TEST_SCENE, "TestNode3D", "rotation")

    data2 = await ctx.session.call_tool("get_node_rotation_3d", {"node_path": "TestNode3D"})
    parsed2 = _parse(data2)
    return TestResult("set_node_rotation_3d", "PASS",
                      expected="rot=15,30,45", actual=parsed2,
                      disk_verified=disk_rot is not None,
                      disk_detail=f"disk rotation={disk_rot}")


async def test_get_node_scale_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_scale_3d", {"node_path": "TestNode3D"})
    parsed = _parse(data)
    return TestResult("get_node_scale_3d", "PASS" if parsed else "FAIL",
                      expected="scale {x,y,z}", actual=parsed)


async def test_set_node_scale_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_scale_3d", {
        "node_path": "TestNode3D", "scale_x": 2, "scale_y": 3, "scale_z": 4,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_scale = get_node_property(TEST_SCENE, "TestNode3D", "scale")

    data2 = await ctx.session.call_tool("get_node_scale_3d", {"node_path": "TestNode3D"})
    parsed2 = _parse(data2)
    return TestResult("set_node_scale_3d", "PASS",
                      expected="scale=2,3,4", actual=parsed2,
                      disk_verified=disk_scale is not None,
                      disk_detail=f"disk scale={disk_scale}")


async def test_set_node_transform_3d(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_transform_3d", {
        "node_path": "TestNode3D",
        "x": 50, "y": 60, "z": 70,
        "rot_x": 10, "rot_y": 20, "rot_z": 30,
        "scale_x": 1, "scale_y": 1, "scale_z": 1,
    })
    parsed = _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_pos = get_node_property(TEST_SCENE, "TestNode3D", "position")
    return TestResult("set_node_transform_3d", "PASS" if not parsed.get("error") else "FAIL",
                      expected="transform set", actual=parsed,
                      disk_verified=disk_pos is not None,
                      disk_detail=f"disk pos={disk_pos}")


def _parse(result):
    content = result.content if hasattr(result, "content") else []
    for c in content:
        text = getattr(c, "text", str(c))
        if text and text.startswith("{"):
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                pass
        return {"_raw": text}
    return {}
TOOL_TESTS = [
    ToolTest("get_node_position_3d", run=test_get_node_position_3d, setup=_setup_3d),
    ToolTest("set_node_position_3d", run=test_set_node_position_3d),
    ToolTest("get_node_rotation_3d", run=test_get_node_rotation_3d),
    ToolTest("set_node_rotation_3d", run=test_set_node_rotation_3d),
    ToolTest("get_node_scale_3d", run=test_get_node_scale_3d),
    ToolTest("set_node_scale_3d", run=test_set_node_scale_3d),
    ToolTest("set_node_transform_3d", run=test_set_node_transform_3d, teardown=_teardown_3d),
]
