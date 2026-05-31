import json

from .base import ToolTest, TestResult

TEST_SCENE = "res://test_mcp_find/test_find_scene.tscn"


async def _setup_find(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Node",
        "root_name": "FindRoot",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": TEST_SCENE})
    for name in ["FindMe", "FindMeToo", "Ordinary"]:
        await ctx.session.call_tool("create_node", {
            "name": name,
            "node_type": "Node2D",
            "parent_path": ".",
        })
    await ctx.session.call_tool("add_node_to_group", {
        "node_path": "FindMe", "group": "searchable_group",
    })
    ctx.temp_scene_path = TEST_SCENE
    ctx.created_nodes.extend(["FindMe", "FindMeToo", "Ordinary"])


async def _teardown_find(ctx):
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene", {"path": TEST_SCENE})
    except Exception:
        pass


async def test_find_nodes_by_name(ctx) -> TestResult:
    data = await ctx.session.call_tool("find_nodes_by_name", {
        "pattern": "FindMe",
        "max_results": 10,
    })
    parsed = _parse(data)
    found = _extract_list(parsed)
    return TestResult("find_nodes_by_name",
                      "PASS" if len(found) >= 2 else "FAIL",
                      expected="at least 2 nodes with 'FindMe'",
                      actual=parsed,
                      disk_verified=False)


async def test_find_nodes_by_type(ctx) -> TestResult:
    data = await ctx.session.call_tool("find_nodes_by_type", {
        "node_class": "Node2D",
        "max_results": 10,
    })
    parsed = _parse(data)
    found = _extract_list(parsed)
    return TestResult("find_nodes_by_type",
                      "PASS" if len(found) >= 3 else "FAIL",
                      expected="at least 3 Node2D",
                      actual=parsed,
                      disk_verified=False)


async def test_find_nodes_by_group(ctx) -> TestResult:
    data = await ctx.session.call_tool("find_nodes_by_group", {
        "group": "searchable_group",
        "max_results": 10,
    })
    parsed = _parse(data)
    found = _extract_list(parsed)
    return TestResult("find_nodes_by_group",
                      "PASS" if len(found) >= 1 else "FAIL",
                      expected="at least 1 node in group",
                      actual=parsed,
                      disk_verified=False)


async def test_find_nodes_by_script(ctx) -> TestResult:
    data = await ctx.session.call_tool("find_nodes_by_script", {
        "script_path": "res://icon.svg",
        "max_results": 10,
    })
    parsed = _parse(data)
    return TestResult("find_nodes_by_script", "PASS",
                      expected="search result (may be empty)", actual=parsed,
                      disk_verified=False)


def _parse(result):
    content = result.content if hasattr(result, "content") else []
    for c in content:
        text = getattr(c, "text", str(c))
        if text and (text.startswith("{") or text.startswith("[")):
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                pass
        return {"_raw": text}
    return {}


def _extract_list(parsed):
    if isinstance(parsed, list):
        return parsed
    if isinstance(parsed, dict):
        for v in parsed.values():
            if isinstance(v, list):
                return v
    return []
TOOL_TESTS = [
    ToolTest("find_nodes_by_name", run=test_find_nodes_by_name, setup=_setup_find),
    ToolTest("find_nodes_by_type", run=test_find_nodes_by_type),
    ToolTest("find_nodes_by_group", run=test_find_nodes_by_group),
    ToolTest("find_nodes_by_script", run=test_find_nodes_by_script, teardown=_teardown_find),
]
