import json

from .base import ToolTest, TestResult

TEST_SCENE = "res://test_mcp_editor_ctrl/test_play_scene.tscn"



async def _setup_play(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Node2D",
        "root_name": "PlayRoot",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": TEST_SCENE})
    await ctx.session.call_tool("set_main_scene", {"scene_path": TEST_SCENE})
    await ctx.session.call_tool("save_scene", {})


async def _teardown_play(ctx):
    if ctx.is_playing:
        try:
            await ctx.session.call_tool("stop_scene", {})
            ctx.is_playing = False
        except Exception:
            pass
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene", {"path": TEST_SCENE})
    except Exception:
        pass


async def test_play_current_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("play_current_scene", {})
    parsed = _parse(data)
    ctx.is_playing = not parsed.get("error")
    return TestResult("play_current_scene",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="scene playing", actual=parsed)


async def test_play_main_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("play_main_scene", {})
    parsed = _parse(data)
    playing = parsed.get("playing", False)
    if playing:
        ctx.is_playing = True
    return TestResult("play_main_scene",
                      "PASS" if playing else "FAIL",
                      expected="main scene playing", actual=parsed)


async def test_is_scene_playing_while_playing(ctx) -> TestResult:
    data = await ctx.session.call_tool("is_scene_playing", {})
    parsed = _parse(data)
    playing = parsed.get("playing", parsed.get("is_playing", parsed.get("_raw", "?")))
    return TestResult("is_scene_playing",
                      "PASS" if str(playing) == "true" or playing is True else "FAIL",
                      expected="is_playing=true", actual=parsed)


async def test_stop_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("stop_scene", {})
    parsed = _parse(data)
    ctx.is_playing = False
    return TestResult("stop_scene",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="scene stopped", actual=parsed)


async def test_is_scene_playing_after_stop(ctx) -> TestResult:
    data = await ctx.session.call_tool("is_scene_playing", {})
    parsed = _parse(data)
    playing = parsed.get("playing", parsed.get("is_playing", parsed.get("_raw", "?")))
    return TestResult("is_scene_playing",
                      "PASS" if str(playing) == "false" or playing is False else "FAIL",
                      expected="is_playing=false", actual=parsed)


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
    ToolTest("play_current_scene", run=test_play_current_scene, setup=_setup_play),
    ToolTest("play_main_scene", run=test_play_main_scene),
    ToolTest("is_scene_playing", run=test_is_scene_playing_while_playing),
    ToolTest("stop_scene", run=test_stop_scene),
    ToolTest("is_scene_playing", run=test_is_scene_playing_after_stop, teardown=_teardown_play),
]
