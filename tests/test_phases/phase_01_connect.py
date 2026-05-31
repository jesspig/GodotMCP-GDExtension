import json

from .base import ToolTest, TestResult


async def test_ping(ctx) -> TestResult:
    data = await ctx.session.call_tool("ping", {})
    parsed = _parse(data)
    return TestResult("ping", "PASS", expected="pong/ok response", actual=parsed,
                      disk_verified=False)


async def test_get_engine_version(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_engine_version", {})
    parsed = _parse(data)
    version = parsed.get("engine_version", parsed.get("_raw", ""))
    if not version or not isinstance(version, str):
        return TestResult("get_engine_version", "FAIL", expected="str version",
                          actual=parsed, error=f"unexpected type: {type(parsed)}")
    ctx.engine_version = version
    return TestResult("get_engine_version", "PASS", expected="version string",
                      actual=version)


async def test_get_plugin_version(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_plugin_version", {})
    parsed = _parse(data)
    version = parsed.get("plugin_version", parsed.get("_raw", ""))
    return TestResult("get_plugin_version", "PASS", expected="version string",
                      actual=version)


async def test_get_editor_info(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_editor_info", {})
    parsed = _parse(data)
    engine_ver = parsed.get("engine_version", "") or parsed.get("version", "")
    if not engine_ver:
        return TestResult("get_editor_info", "FAIL",
                          expected="dict with engine_version", actual=parsed,
                          error="missing engine_version")
    return TestResult("get_editor_info", "PASS", expected="editor info dict",
                      actual=parsed)


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
    ToolTest("ping", run=test_ping),
    ToolTest("get_engine_version", run=test_get_engine_version),
    ToolTest("get_plugin_version", run=test_get_plugin_version),
    ToolTest("get_editor_info", run=test_get_editor_info),
]