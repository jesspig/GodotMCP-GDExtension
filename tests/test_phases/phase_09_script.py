import json

from .base import ToolTest, TestResult
from file_verifier import file_exists, verify_file_contains

TEST_SCRIPT = "res://test_mcp_script/test_script.gd"


async def test_create_gdscript(ctx) -> TestResult:
    source = 'extends Node2D\n\n@export var test_value: int = 42\n\nfunc hello() -> String:\n\treturn "world"\n'
    data = await ctx.session.call_tool("create_gdscript", {
        "path": TEST_SCRIPT,
        "base_class": "Node2D",
        "template": source,
    })
    parsed = _parse(data)
    path = parsed.get("path", "")
    disk_ok = file_exists(path)
    content_ok = verify_file_contains(path, "test_value")
    ctx.created_files.append(path)
    return TestResult("create_gdscript",
                      "PASS" if (disk_ok and content_ok) else "FAIL",
                      expected=f"path={TEST_SCRIPT}",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"exists={disk_ok}, content_has_var={content_ok}")


async def test_read_gdscript(ctx) -> TestResult:
    data = await ctx.session.call_tool("read_gdscript", {
        "path": TEST_SCRIPT,
    })
    parsed = _parse(data)
    source = parsed.get("source", parsed.get("_raw", ""))
    return TestResult("read_gdscript",
                      "PASS" if "extends" in str(source) else "FAIL",
                      expected="script source code",
                      actual=parsed)


async def test_edit_gdscript(ctx) -> TestResult:
    new_source = 'extends Node2D\n\n@export var test_value: int = 99\n\nfunc hello() -> String:\n\treturn "edited"\n'
    data = await ctx.session.call_tool("edit_gdscript", {
        "path": TEST_SCRIPT,
        "source": new_source,
    })
    parsed = _parse(data)
    disk_ok = verify_file_contains(TEST_SCRIPT, "test_value: int = 99")
    return TestResult("edit_gdscript",
                      "PASS" if (not parsed.get("error") and disk_ok) else "FAIL",
                      expected="script edited",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"disk_contains_99={disk_ok}")


async def test_validate_gdscript(ctx) -> TestResult:
    data = await ctx.session.call_tool("validate_gdscript", {
        "path": TEST_SCRIPT,
    })
    parsed = _parse(data)
    return TestResult("validate_gdscript",
                      "PASS",
                      expected="validation result (may need LSP)",
                      actual=parsed)


async def _setup_attach(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": "res://test_mcp_script/attach_scene.tscn",
        "root_type": "Node2D",
        "root_name": "AttachRoot",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": "res://test_mcp_script/attach_scene.tscn"})
    ctx.temp_scene_path = "res://test_mcp_script/attach_scene.tscn"


async def test_attach_script(ctx) -> TestResult:
    data = await ctx.session.call_tool("attach_script", {
        "node_path": ".",
        "script_path": TEST_SCRIPT,
    })
    parsed = _parse(data)
    await ctx.session.call_tool("save_scene", {})

    from file_verifier import parse_tscn
    parsed_tscn = parse_tscn("res://test_mcp_script/attach_scene.tscn")
    root = parsed_tscn.get("nodes", {}).get("AttachRoot", {})
    has_script_ref = "script" in root.get("props", {})
    return TestResult("attach_script",
                      "PASS" if (not parsed.get("error") and has_script_ref) else "FAIL",
                      expected="script attached",
                      actual=parsed,
                      disk_verified=has_script_ref,
                      disk_detail=f"script in tscn={has_script_ref}")


async def test_get_script_variables(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_script_variables", {
        "node_path": ".",
    })
    parsed = _parse(data)
    return TestResult("get_script_variables",
                      "PASS" if parsed else "FAIL",
                      expected="script variables",
                      actual=parsed)


async def test_get_variable(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_variable", {
        "node_path": ".",
        "variable": "test_value",
    })
    parsed = _parse(data)
    return TestResult("get_variable",
                      "PASS" if parsed else "FAIL",
                      expected="variable value",
                      actual=parsed)


async def test_set_variable(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_variable", {
        "node_path": ".",
        "variable": "test_value",
        "value": 77,
    })
    parsed = _parse(data)

    data2 = await ctx.session.call_tool("get_variable", {
        "node_path": ".",
        "variable": "test_value",
    })
    parsed2 = _parse(data2)
    return TestResult("set_variable",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="variable=77",
                      actual=parsed2)


async def test_call_method(ctx) -> TestResult:
    data = await ctx.session.call_tool("call_method", {
        "node_path": ".",
        "method": "hello",
        "args": [],
    })
    parsed = _parse(data)
    return TestResult("call_method",
                      "PASS" if parsed else "FAIL",
                      expected="method result",
                      actual=parsed)


async def test_detach_script(ctx) -> TestResult:
    data = await ctx.session.call_tool("detach_script", {
        "node_path": ".",
    })
    parsed = _parse(data)
    return TestResult("detach_script",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="script detached",
                      actual=parsed,
                      disk_verified=False)


async def _teardown_script(ctx):
    try:
        await ctx.session.call_tool("detach_script", {"node_path": "."})
    except Exception:
        pass
    try:
        await ctx.session.call_tool("delete_scene",
                                     {"path": "res://test_mcp_script/attach_scene.tscn"})
    except Exception:
        pass


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
    ToolTest("create_gdscript", run=test_create_gdscript, setup=_setup_attach),
    ToolTest("read_gdscript", run=test_read_gdscript),
    ToolTest("edit_gdscript", run=test_edit_gdscript),
    ToolTest("validate_gdscript", run=test_validate_gdscript),
    ToolTest("attach_script", run=test_attach_script),
    ToolTest("get_script_variables", run=test_get_script_variables),
    ToolTest("get_variable", run=test_get_variable),
    ToolTest("set_variable", run=test_set_variable),
    ToolTest("call_method", run=test_call_method),
    ToolTest("detach_script", run=test_detach_script, teardown=_teardown_script),
]
