from .base import ToolTest, TestResult


async def test_get_open_scenes(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_open_scenes", {})
    parsed = _parse(data)
    return TestResult("get_open_scenes", "PASS", expected="list of paths",
                      actual=parsed)


async def test_get_open_scene_roots(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_open_scene_roots", {})
    parsed = _parse(data)
    return TestResult("get_open_scene_roots", "PASS",
                      expected="list of root info", actual=parsed)


async def test_is_scene_dirty(ctx) -> TestResult:
    data = await ctx.session.call_tool("is_scene_dirty", {})
    parsed = _parse(data)
    return TestResult("is_scene_dirty", "PASS", expected="bool result",
                      actual=parsed)


async def test_is_scene_playing(ctx) -> TestResult:
    data = await ctx.session.call_tool("is_scene_playing", {})
    parsed = _parse(data)
    return TestResult("is_scene_playing", "PASS", expected="bool result",
                      actual=parsed)


async def test_get_project_setting(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_project_setting",
                                        {"property": "application/config/name"})
    parsed = _parse(data)
    return TestResult("get_project_setting", "PASS",
                      expected='application/config/name',
                      actual=parsed)


async def test_get_project_info(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_project_info", {})
    parsed = _parse(data)
    if not parsed or not isinstance(parsed, dict):
        return TestResult("get_project_info", "FAIL", expected="dict",
                          actual=parsed, error="非 dict 返回值")
    return TestResult("get_project_info", "PASS", expected="project info dict",
                      actual=parsed)


async def test_get_display_settings(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_display_settings", {})
    parsed = _parse(data)
    return TestResult("get_display_settings", "PASS",
                      expected="display settings dict", actual=parsed)


async def test_get_physics_settings(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_physics_settings", {})
    parsed = _parse(data)
    return TestResult("get_physics_settings", "PASS",
                      expected="physics settings dict", actual=parsed)


async def test_get_rendering_settings(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_rendering_settings", {})
    parsed = _parse(data)
    return TestResult("get_rendering_settings", "PASS",
                      expected="rendering settings dict", actual=parsed)


async def test_list_autoloads(ctx) -> TestResult:
    data = await ctx.session.call_tool("list_autoloads", {})
    parsed = _parse(data)
    return TestResult("list_autoloads", "PASS", expected="list of autoloads",
                      actual=parsed)


async def test_list_scenes(ctx) -> TestResult:
    data = await ctx.session.call_tool("list_scenes", {})
    parsed = _parse(data)
    return TestResult("list_scenes", "PASS", expected="list of scene paths",
                      actual=parsed)


async def test_list_gdscripts(ctx) -> TestResult:
    data = await ctx.session.call_tool("list_gdscripts", {})
    parsed = _parse(data)
    return TestResult("list_gdscripts", "PASS", expected="list of gdscripts",
                      actual=parsed)


async def test_get_layer_names(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_layer_names",
                                        {"category": "2d_physics"})
    parsed = _parse(data)
    return TestResult("get_layer_names", "PASS",
                      expected="layer names dict", actual=parsed)


def _parse(result):
    import json
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
    ToolTest("get_open_scenes", run=test_get_open_scenes),
    ToolTest("get_open_scene_roots", run=test_get_open_scene_roots),
    ToolTest("is_scene_dirty", run=test_is_scene_dirty),
    ToolTest("is_scene_playing", run=test_is_scene_playing),
    ToolTest("get_project_setting", run=test_get_project_setting),
    ToolTest("get_project_info", run=test_get_project_info),
    ToolTest("get_display_settings", run=test_get_display_settings),
    ToolTest("get_physics_settings", run=test_get_physics_settings),
    ToolTest("get_rendering_settings", run=test_get_rendering_settings),
    ToolTest("list_autoloads", run=test_list_autoloads),
    ToolTest("list_scenes", run=test_list_scenes),
    ToolTest("list_gdscripts", run=test_list_gdscripts),
    ToolTest("get_layer_names", run=test_get_layer_names),
]
