import json

from .base import ToolTest, TestResult
from file_verifier import verify_file_contains

TEMP_FILE = "res://test_mcp_search/search_target.gd"



async def _setup_search(ctx):
    source = "# Test file for search\nvar searchable_value = 123\nvar another_value = 456\n"
    await ctx.session.call_tool("create_gdscript", {
        "path": TEMP_FILE,
        "template": source,
        "base_class": "Node",
    })
    ctx.created_files.append(TEMP_FILE)


async def _teardown_search(ctx):
    pass


async def test_find_in_file(ctx) -> TestResult:
    data = await ctx.session.call_tool("find_in_file", {
        "path": TEMP_FILE,
        "pattern": "searchable_value",
        "literal": True,
        "max_matches": 5,
    })
    parsed = _parse(data)
    return TestResult("find_in_file", "PASS" if parsed else "FAIL",
                      expected="matches for 'searchable_value'",
                      actual=parsed)


async def test_search_project(ctx) -> TestResult:
    data = await ctx.session.call_tool("search_project", {
        "pattern": "searchable_value",
        "literal": True,
        "max_matches": 10,
    })
    parsed = _parse(data)
    return TestResult("search_project", "PASS" if parsed else "FAIL",
                      expected="project-wide search results",
                      actual=parsed)


async def test_find_and_replace(ctx) -> TestResult:
    data = await ctx.session.call_tool("find_and_replace", {
        "path": TEMP_FILE,
        "pattern": "another_value",
        "replacement": "replaced_value",
        "literal": True,
    })
    parsed = _parse(data)
    disk_ok = verify_file_contains(TEMP_FILE, "replaced_value")
    old_gone = not verify_file_contains(TEMP_FILE, "another_value")\
               if disk_ok else False
    return TestResult("find_and_replace",
                      "PASS" if (not parsed.get("error") and old_gone) else "FAIL",
                      expected="replaced 'another_value' with 'replaced_value'",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"has_replaced={disk_ok}, has_old={not old_gone}")


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
    ToolTest("find_in_file", run=test_find_in_file, setup=_setup_search),
    ToolTest("search_project", run=test_search_project),
    ToolTest("find_and_replace", run=test_find_and_replace, teardown=_teardown_search),
]
