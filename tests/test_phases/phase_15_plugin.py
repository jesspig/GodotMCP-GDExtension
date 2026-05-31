import json

from .base import ToolTest, TestResult



async def test_list_plugins(ctx) -> TestResult:
    data = await ctx.session.call_tool("list_plugins", {})
    parsed = _parse(data)
    return TestResult("list_plugins", "PASS" if parsed else "FAIL",
                      expected="list of plugins", actual=parsed)


async def test_set_plugin_enabled(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_plugin_enabled", {
        "plugin": "godot_mcp",
        "enabled": True,
    })
    parsed = _parse(data)

    data2 = await ctx.session.call_tool("list_plugins", {})
    parsed2 = _parse(data2)
    return TestResult("set_plugin_enabled",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="plugin toggled", actual=parsed2)


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


TOOL_TESTS = [
    ToolTest("list_plugins", run=test_list_plugins),
    ToolTest("set_plugin_enabled", run=test_set_plugin_enabled),
]