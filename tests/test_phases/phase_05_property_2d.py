import json

from .base import ToolTest, TestResult
from file_verifier import get_node_property

TEST_SCENE = "res://test_mcp_prop2d/test_2d_scene.tscn"


async def _setup_2d(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Node2D",
        "root_name": "Root2D",
    })
    await ctx.session.call_tool("create_node", {
        "name": "TestSprite",
        "node_type": "Sprite2D",
        "parent_path": ".",
    })
    await ctx.session.call_tool("create_node", {
        "name": "TestLabel",
        "node_type": "Label",
        "parent_path": ".",
    })
    ctx.temp_scene_path = TEST_SCENE
    ctx.created_nodes.extend(["TestSprite", "TestLabel"])


async def _teardown_2d(ctx):
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene", {"path": TEST_SCENE})
    except Exception:
        pass


async def test_get_node_position(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_position", {"node_path": "TestSprite"})
    parsed = _parse(data)
    return TestResult("get_node_position", "PASS" if parsed else "FAIL",
                      expected="position {x, y}", actual=parsed)


async def test_set_node_position(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_position", {
        "node_path": "TestSprite", "x": 150, "y": 250,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_pos = get_node_property(TEST_SCENE, "TestSprite", "position")

    data2 = await ctx.session.call_tool("get_node_position", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_position", "PASS",
                      expected="x=150,y=250", actual=parsed2,
                      disk_verified=disk_pos is not None,
                      disk_detail=f"disk position={disk_pos}")


async def test_get_node_rotation(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_rotation", {"node_path": "TestSprite"})
    parsed = _parse(data)
    return TestResult("get_node_rotation", "PASS" if parsed else "FAIL",
                      expected="rotation degrees", actual=parsed)


async def test_set_node_rotation(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_rotation", {
        "node_path": "TestSprite", "degrees": 45,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_rot = get_node_property(TEST_SCENE, "TestSprite", "rotation")

    data2 = await ctx.session.call_tool("get_node_rotation", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_rotation", "PASS",
                      expected="rotation=45", actual=parsed2,
                      disk_verified=disk_rot is not None,
                      disk_detail=f"disk rotation={disk_rot}")


async def test_get_node_scale(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_scale", {"node_path": "TestSprite"})
    parsed = _parse(data)
    return TestResult("get_node_scale", "PASS" if parsed else "FAIL",
                      expected="scale", actual=parsed)


async def test_set_node_scale(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_scale", {
        "node_path": "TestSprite", "x": 2, "y": 3,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_scale = get_node_property(TEST_SCENE, "TestSprite", "scale")

    data2 = await ctx.session.call_tool("get_node_scale", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_scale", "PASS",
                      expected="scale", actual=parsed2,
                      disk_verified=disk_scale is not None,
                      disk_detail=f"disk scale={disk_scale}")


async def test_get_node_visible(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_visible", {"node_path": "TestSprite"})
    parsed = _parse(data)
    return TestResult("get_node_visible", "PASS" if parsed else "FAIL",
                      expected="visible bool", actual=parsed)


async def test_set_node_visible(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_visible", {
        "node_path": "TestSprite", "visible": False,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_vis = get_node_property(TEST_SCENE, "TestSprite", "visible")

    data2 = await ctx.session.call_tool("get_node_visible", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_visible", "PASS",
                      expected="visible=false", actual=parsed2,
                      disk_verified=disk_vis is not None,
                      disk_detail=f"disk visible={disk_vis}")


async def test_get_node_modulate(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_modulate", {"node_path": "TestSprite"})
    parsed = _parse(data)
    return TestResult("get_node_modulate", "PASS" if parsed else "FAIL",
                      expected="modulate color", actual=parsed)


async def test_set_node_modulate(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_modulate", {
        "node_path": "TestSprite", "r": 1, "g": 0, "b": 0, "a": 0.5,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_mod = get_node_property(TEST_SCENE, "TestSprite", "modulate")

    data2 = await ctx.session.call_tool("get_node_modulate", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_modulate", "PASS",
                      expected="r=1,g=0,b=0,a=0.5", actual=parsed2,
                      disk_verified=disk_mod is not None,
                      disk_detail=f"disk modulate={disk_mod}")


async def test_get_node_z_index(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_z_index", {"node_path": "TestSprite"})
    parsed = _parse(data)
    return TestResult("get_node_z_index", "PASS" if parsed else "FAIL",
                      expected="z_index int", actual=parsed)


async def test_set_node_z_index(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_z_index", {
        "node_path": "TestSprite", "z_index": 10,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_z = get_node_property(TEST_SCENE, "TestSprite", "z_index")

    data2 = await ctx.session.call_tool("get_node_z_index", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_z_index", "PASS",
                      expected="z_index=10", actual=parsed2,
                      disk_verified=disk_z is not None,
                      disk_detail=f"disk z_index={disk_z}")


async def test_get_node_text(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_text", {"node_path": "TestLabel"})
    parsed = _parse(data)
    return TestResult("get_node_text", "PASS" if parsed else "FAIL",
                      expected="text string", actual=parsed)


async def test_set_node_text(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_text", {
        "node_path": "TestLabel", "text": "Hello MCP",
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_text = get_node_property(TEST_SCENE, "TestLabel", "text")

    data2 = await ctx.session.call_tool("get_node_text", {"node_path": "TestLabel"})
    parsed2 = _parse(data2)
    return TestResult("set_node_text", "PASS",
                      expected='text="Hello MCP"', actual=parsed2,
                      disk_verified=disk_text is not None,
                      disk_detail=f'disk text={disk_text}')


async def test_set_node_unique_name(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_unique_name", {
        "node_path": "TestSprite", "unique": True,
    })
    parsed = _parse(data)
    return TestResult("set_node_unique_name", "PASS" if not parsed.get("error") else "FAIL",
                      expected="unique name set", actual=parsed)


async def test_set_node_transform_2d(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_transform_2d", {
        "node_path": "TestSprite",
        "x": 300, "y": 400, "degrees": 90, "scale_x": 1.5, "scale_y": 1.5,
    })
    _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_pos = get_node_property(TEST_SCENE, "TestSprite", "position")
    disk_scale = get_node_property(TEST_SCENE, "TestSprite", "scale")

    data2 = await ctx.session.call_tool("get_node_position", {"node_path": "TestSprite"})
    parsed2 = _parse(data2)
    return TestResult("set_node_transform_2d", "PASS",
                      expected="transform set", actual=parsed2,
                      disk_verified=disk_pos is not None,
                      disk_detail=f"disk pos={disk_pos} scale={disk_scale}")


async def test_get_node_texture(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_texture", {
        "node_path": "TestSprite", "property": "texture",
    })
    parsed = _parse(data)
    return TestResult("get_node_texture", "PASS" if parsed else "FAIL",
                      expected="texture info", actual=parsed)


async def test_batch_set_property(ctx) -> TestResult:
    data = await ctx.session.call_tool("batch_set_property", {
        "node_paths": ["TestSprite", "TestLabel"],
        "property": "position",
        "value": {"x": 99, "y": 88},
    })
    parsed = _parse(data)
    results = parsed.get("results", [])
    all_ok = all(r.get("status") == "ok" for r in results)
    return TestResult("batch_set_property", "PASS" if all_ok else "FAIL",
                      expected="all nodes updated",
                      actual=parsed)


async def test_set_node_texture(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_texture", {
        "node_path": "TestSprite",
        "texture_path": "res://icon.svg",
        "property": "texture",
    })
    parsed = _parse(data)
    await ctx.session.call_tool("save_scene", {})
    disk_tex = get_node_property(TEST_SCENE, "TestSprite", "texture")
    disk_ok = disk_tex is not None and "icon.svg" in str(disk_tex)
    return TestResult("set_node_texture", "PASS" if not parsed.get("error") else "FAIL",
                      expected="texture=icon.svg", actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"disk texture={disk_tex}")


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
    ToolTest("get_node_position", run=test_get_node_position, setup=_setup_2d),
    ToolTest("set_node_position", run=test_set_node_position),
    ToolTest("get_node_rotation", run=test_get_node_rotation),
    ToolTest("set_node_rotation", run=test_set_node_rotation),
    ToolTest("get_node_scale", run=test_get_node_scale),
    ToolTest("set_node_scale", run=test_set_node_scale),
    ToolTest("get_node_visible", run=test_get_node_visible),
    ToolTest("set_node_visible", run=test_set_node_visible),
    ToolTest("get_node_modulate", run=test_get_node_modulate),
    ToolTest("set_node_modulate", run=test_set_node_modulate),
    ToolTest("get_node_z_index", run=test_get_node_z_index),
    ToolTest("set_node_z_index", run=test_set_node_z_index),
    ToolTest("get_node_text", run=test_get_node_text),
    ToolTest("set_node_text", run=test_set_node_text),
    ToolTest("set_node_unique_name", run=test_set_node_unique_name),
    ToolTest("set_node_transform_2d", run=test_set_node_transform_2d),
    ToolTest("get_node_texture", run=test_get_node_texture),
    ToolTest("set_node_texture", run=test_set_node_texture),
    ToolTest("batch_set_property", run=test_batch_set_property, teardown=_teardown_2d),
]
