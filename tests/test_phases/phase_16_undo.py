import json

from .base import ToolTest, TestResult
from file_verifier import verify_tscn_has_node

TEST_SCENE = "res://test_mcp_undo/test_undo_scene.tscn"



async def _setup_undo(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Node2D",
        "root_name": "UndoRoot",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": TEST_SCENE})
    await ctx.session.call_tool("create_node", {
        "name": "UndoMe",
        "node_type": "Sprite2D",
        "parent_path": ".",
    })
    await ctx.session.call_tool("save_scene", {})


async def _teardown_undo(ctx):
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene", {"path": TEST_SCENE})
    except Exception:
        pass


async def test_undo(ctx) -> TestResult:
    data = await ctx.session.call_tool("undo", {})
    parsed = _parse(data)

    await ctx.session.call_tool("save_scene", {})
    node_gone = not verify_tscn_has_node(TEST_SCENE, "UndoMe", "Sprite2D")
    return TestResult("undo",
                      "PASS" if (not parsed.get("error") and node_gone) else "FAIL",
                      expected="undo creates 'UndoMe' node",
                      actual=parsed,
                      disk_verified=node_gone,
                      disk_detail=f"node_gone_after_undo={node_gone}")


async def test_redo(ctx) -> TestResult:
    data = await ctx.session.call_tool("redo", {})
    parsed = _parse(data)

    await ctx.session.call_tool("save_scene", {})
    node_back = verify_tscn_has_node(TEST_SCENE, "UndoMe", "Sprite2D")
    return TestResult("redo",
                      "PASS" if (not parsed.get("error") and node_back) else "FAIL",
                      expected="redo restores 'UndoMe' node",
                      actual=parsed,
                      disk_verified=node_back,
                      disk_detail=f"node_back_after_redo={node_back}")


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
    ToolTest("undo", run=test_undo, setup=_setup_undo),
    ToolTest("redo", run=test_redo, teardown=_teardown_undo),
]
