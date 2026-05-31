import json

from .base import ToolTest, TestResult
from file_verifier import read_project_godot_value, read_project_godot_raw


ORIGINALS: dict = {}


async def _record_originals(ctx):
    ORIGINALS["project_godot"] = read_project_godot_raw()
    for key in ["application/config/name",
                "application/config/main_scene",
                "application/config/description"]:
        ORIGINALS[key] = read_project_godot_value(
            key.split("/")[0], key, ctx.project_path)


async def _restore_originals(ctx):
    try:
        if ORIGINALS.get("project_godot"):
            import os as _os
            path = _os.path.join(ctx.project_path, "project.godot")
            with open(path, "w", encoding="utf-8") as f:
                f.write(ORIGINALS["project_godot"])
    except Exception:
        pass


async def test_set_project_setting(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_project_setting", {
        "property": "application/config/name",
        "value": "MCP Test Project",
    })
    parsed = _parse(data)
    cfg_name = read_project_godot_value("application", "config/name",
                                         ctx.project_path)
    disk_ok = cfg_name is not None and "MCP Test" in cfg_name
    return TestResult("set_project_setting",
                      "PASS" if disk_ok else "FAIL",
                      expected='name="MCP Test Project"',
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f'disk config/name={cfg_name}')


async def test_set_project_info(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_project_info", {
        "name": "TestMCP Project",
        "description": "MCP test description",
    })
    parsed = _parse(data)
    cfg_name = read_project_godot_value("application", "config/name",
                                         ctx.project_path)
    disk_ok = cfg_name is not None and "TestMCP" in cfg_name
    return TestResult("set_project_info", "PASS" if disk_ok else "FAIL",
                      expected='name="TestMCP Project"', actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f'disk config/name={cfg_name}')


async def test_set_main_scene(ctx) -> TestResult:
    # Use the scene created by an earlier phase, or fall back to empty/clear
    scene_path = getattr(ctx, "temp_scene_path", "")
    data = await ctx.session.call_tool("set_main_scene", {
        "scene_path": scene_path,
    })
    parsed = _parse(data)
    main_scene = read_project_godot_value("application",
                                           "config/main_scene",
                                           ctx.project_path)
    return TestResult("set_main_scene",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="main scene set/cleared", actual=parsed,
                      disk_verified=False)


async def test_add_autoload(ctx) -> TestResult:
    autoload_path = getattr(ctx, "temp_scene_path", "res://icon.svg")
    data = await ctx.session.call_tool("add_autoload", {
        "name": "MCPTestAutoload",
        "path": autoload_path,
    })
    parsed = _parse(data)
    ctx.test_autoloads.append("MCPTestAutoload")
    return TestResult("add_autoload",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="autoload added", actual=parsed)


async def test_remove_autoload(ctx) -> TestResult:
    data = await ctx.session.call_tool("remove_autoload", {
        "name": "MCPTestAutoload",
    })
    parsed = _parse(data)
    if "MCPTestAutoload" in ctx.test_autoloads:
        ctx.test_autoloads.remove("MCPTestAutoload")
    return TestResult("remove_autoload",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="autoload removed", actual=parsed)


async def test_set_display_settings(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_display_settings", {
        "viewport_width": 1024,
        "viewport_height": 768,
    })
    parsed = _parse(data)
    return TestResult("set_display_settings",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="display settings set", actual=parsed)


async def test_set_physics_settings(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_physics_settings", {
        "gravity_2d": 980,
    })
    parsed = _parse(data)
    return TestResult("set_physics_settings",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="physics settings set", actual=parsed)


async def test_set_rendering_settings(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_rendering_settings", {
        "default_clear_color": "#336699",
    })
    parsed = _parse(data)
    return TestResult("set_rendering_settings",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="rendering settings set", actual=parsed)


async def test_set_layer_names(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_layer_names", {
        "category": "2d_physics",
        "layers": {"layer_1": "test_layer_mcp"},
    })
    parsed = _parse(data)
    return TestResult("set_layer_names",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="layer names set", actual=parsed)


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
    ToolTest("set_project_setting", run=test_set_project_setting, setup=_record_originals),
    ToolTest("set_project_info", run=test_set_project_info),
    ToolTest("set_main_scene", run=test_set_main_scene),
    ToolTest("add_autoload", run=test_add_autoload),
    ToolTest("remove_autoload", run=test_remove_autoload),
    ToolTest("set_display_settings", run=test_set_display_settings),
    ToolTest("set_physics_settings", run=test_set_physics_settings),
    ToolTest("set_rendering_settings", run=test_set_rendering_settings),
    ToolTest("set_layer_names", run=test_set_layer_names, teardown=_restore_originals),
]
