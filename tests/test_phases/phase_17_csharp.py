import json

from .base import ToolTest, TestResult
from file_verifier import file_exists



async def _teardown_csharp(ctx):
    for f in ["res://MCPTestScript.cs", "res://MCPTestScript2.cs"]:
        try:
            await ctx.session.call_tool("delete_scene", {"path": f})
        except Exception:
            pass


async def test_csharp_create_solution(ctx) -> TestResult:
    data = await ctx.session.call_tool("csharp_create_solution", {})
    parsed = _parse(data)
    sln_exists = file_exists("Example.sln", ctx.project_path)
    csproj_exists = file_exists("Example.csproj", ctx.project_path)
    disk_ok = sln_exists and csproj_exists
    return TestResult("csharp_create_solution",
                      "PASS" if (not parsed.get("error") and disk_ok) else "FAIL",
                      expected=".sln and .csproj created",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"sln={sln_exists}, csproj={csproj_exists}")


async def test_create_csharp_script(ctx) -> TestResult:
    data = await ctx.session.call_tool("create_csharp_script", {
        "path": "res://MCPTestScript.cs",
    })
    parsed = _parse(data)
    disk_ok = file_exists("res://MCPTestScript.cs")
    ctx.created_files.append("res://MCPTestScript.cs")
    return TestResult("create_csharp_script",
                      "PASS" if (not parsed.get("error") and disk_ok) else "FAIL",
                      expected=".cs file created",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"file_exists={disk_ok}")


async def test_read_csharp_script(ctx) -> TestResult:
    data = await ctx.session.call_tool("read_csharp_script", {
        "path": "res://MCPTestScript.cs",
    })
    parsed = _parse(data)
    source = parsed.get("source", parsed.get("_raw", ""))
    return TestResult("read_csharp_script",
                      "PASS" if source else "FAIL",
                      expected="C# source code",
                      actual=parsed)


async def test_edit_csharp_script(ctx) -> TestResult:
    new_source = (
        'using Godot;\n'
        'using System;\n\n'
        'public partial class MCPTestScript : Node\n'
        '{\n'
        '    public override void _Ready()\n'
        '    {\n'
        '        GD.Print("MCP Test Script Loaded");\n'
        '    }\n'
        '}\n'
    )
    data = await ctx.session.call_tool("edit_csharp_script", {
        "path": "res://MCPTestScript.cs",
        "source": new_source,
    })
    parsed = _parse(data)
    return TestResult("edit_csharp_script",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="script edited",
                      actual=parsed)


async def test_csharp_build(ctx) -> TestResult:
    data = await ctx.session.call_tool("csharp_build", {
        "configuration": "Debug",
    })
    parsed = _parse(data)
    return TestResult("csharp_build",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="build succeeded",
                      actual=parsed)


async def test_list_csharp_scripts(ctx) -> TestResult:
    data = await ctx.session.call_tool("list_csharp_scripts", {})
    parsed = _parse(data)
    return TestResult("list_csharp_scripts",
                      "PASS" if parsed else "FAIL",
                      expected="list of C# scripts",
                      actual=parsed)


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
    ToolTest("csharp_create_solution", run=test_csharp_create_solution),
    ToolTest("create_csharp_script", run=test_create_csharp_script),
    ToolTest("read_csharp_script", run=test_read_csharp_script),
    ToolTest("edit_csharp_script", run=test_edit_csharp_script),
    ToolTest("csharp_build", run=test_csharp_build),
    ToolTest("list_csharp_scripts", run=test_list_csharp_scripts, teardown=_teardown_csharp),
]
