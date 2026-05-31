import json

from .base import ToolTest, TestResult
from file_verifier import verify_tscn_has_node

TEST_SCENE = "res://test_mcp_coll/test_collision.tscn"


async def _setup_coll(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Area2D",
        "root_name": "TestArea",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": TEST_SCENE})
    ctx.temp_scene_path = TEST_SCENE


async def _teardown_coll(ctx):
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene", {"path": TEST_SCENE})
    except Exception:
        pass


async def test_add_circle_collision(ctx) -> TestResult:
    data = await ctx.session.call_tool("add_circle_collision", {
        "node_path": ".",
        "radius": 50,
    })
    parsed = _parse(data)
    await ctx.session.call_tool("save_scene", {})
    has_coll = verify_tscn_has_node(TEST_SCENE, "CollisionShape2D")
    return TestResult("add_circle_collision",
                      "PASS" if (not parsed.get("error") and has_coll) else "FAIL",
                      expected="circle collision added",
                      actual=parsed,
                      disk_verified=has_coll,
                      disk_detail=f"CollisionShape2D in tscn={has_coll}")


async def test_get_node_collision_layer(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_collision_layer", {"node_path": "."})
    parsed = _parse(data)
    has_layer = "collision_layer" in parsed
    return TestResult("get_node_collision_layer", "PASS" if has_layer else "FAIL",
                      expected="collision_layer value", actual=parsed)


async def test_set_node_collision_layer(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_collision_layer", {
        "node_path": ".", "layer": 3,
    })
    parsed = _parse(data)
    return TestResult("set_node_collision_layer",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="layer=3", actual=parsed)


async def test_get_node_collision_mask(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_collision_mask", {"node_path": "."})
    parsed = _parse(data)
    has_mask = "collision_mask" in parsed
    return TestResult("get_node_collision_mask", "PASS" if has_mask else "FAIL",
                      expected="collision_mask value", actual=parsed)


async def test_set_node_collision_mask(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_node_collision_mask", {
        "node_path": ".", "mask": 5,
    })
    parsed = _parse(data)
    return TestResult("set_node_collision_mask",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="mask=5", actual=parsed)


async def test_add_rectangle_collision(ctx) -> TestResult:
    data = await ctx.session.call_tool("add_rectangle_collision", {
        "node_path": ".",
        "width": 100,
        "height": 200,
    })
    parsed = _parse(data)
    await ctx.session.call_tool("save_scene", {})
    has_rect = verify_tscn_has_node(TEST_SCENE, "CollisionShape2D")
    return TestResult("add_rectangle_collision",
                      "PASS" if (not parsed.get("error") and has_rect) else "FAIL",
                      expected="rectangle collision added",
                      actual=parsed,
                      disk_verified=has_rect,
                      disk_detail=f"CollisionShape2D in tscn={has_rect}")


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
    ToolTest("add_circle_collision", run=test_add_circle_collision, setup=_setup_coll),
    ToolTest("add_rectangle_collision", run=test_add_rectangle_collision),
    ToolTest("get_node_collision_layer", run=test_get_node_collision_layer),
    ToolTest("set_node_collision_layer", run=test_set_node_collision_layer),
    ToolTest("get_node_collision_mask", run=test_get_node_collision_mask),
    ToolTest("set_node_collision_mask", run=test_set_node_collision_mask, teardown=_teardown_coll),
]
