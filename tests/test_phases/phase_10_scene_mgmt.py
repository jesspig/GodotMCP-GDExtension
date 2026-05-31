import json

from .base import ToolTest, TestResult
from file_verifier import file_exists

TEMP_DIR = "res://test_mcp_scenemgmt"
SCENE_A = f"{TEMP_DIR}/scene_a.tscn"
SCENE_B = f"{TEMP_DIR}/scene_b.tscn"
RENAMED = f"{TEMP_DIR}/scene_renamed.tscn"
TO_DELETE = f"{TEMP_DIR}/to_delete.tscn"


async def _setup_scenes(ctx):
    await ctx.session.call_tool("create_scene", {
        "path": SCENE_A,
        "root_type": "Node2D",
        "root_name": "SceneARoot",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": SCENE_A})
    await ctx.session.call_tool("create_node", {
        "name": "ChildOfA",
        "node_type": "Sprite2D",
        "parent_path": ".",
    })
    await ctx.session.call_tool("save_scene", {})


async def _teardown_all(ctx):
    try:
        await ctx.session.call_tool("close_scene", {})
    except Exception:
        pass
    for path in [SCENE_A, SCENE_B, RENAMED, f"{TEMP_DIR}/branched.tscn", TO_DELETE]:
        try:
            await ctx.session.call_tool("delete_scene", {"path": path})
        except Exception:
            pass


async def test_open_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("open_scene", {"scene_path": SCENE_A})
    parsed = _parse(data)
    ctx.temp_scene_path = SCENE_A
    return TestResult("open_scene", "PASS" if not parsed.get("error") else "FAIL",
                      expected="scene opened", actual=parsed)


async def test_refresh_filesystem(ctx) -> TestResult:
    data = await ctx.session.call_tool("refresh_filesystem", {})
    parsed = _parse(data)
    return TestResult("refresh_filesystem", "PASS" if parsed.get("scanning") else "FAIL",
                      expected="scanning=true", actual=parsed)


async def test_delete_scene(ctx) -> TestResult:
    await ctx.session.call_tool("create_scene", {
        "path": TO_DELETE,
        "root_type": "Node2D",
        "root_name": "ToDelete",
    })
    await ctx.session.call_tool("open_scene", {"scene_path": TO_DELETE})
    await ctx.session.call_tool("save_scene", {})
    data = await ctx.session.call_tool("delete_scene", {"path": TO_DELETE})
    parsed = _parse(data)
    deleted = parsed.get("deleted", "")
    return TestResult("delete_scene", "PASS" if deleted else "FAIL",
                      expected="scene deleted", actual=parsed)


async def test_save_scene_as(ctx) -> TestResult:
    data = await ctx.session.call_tool("save_scene_as", {"scene_path": SCENE_B})
    parsed = _parse(data)
    disk_ok = file_exists(SCENE_B)
    return TestResult("save_scene_as", "PASS" if disk_ok else "FAIL",
                      expected=f"path={SCENE_B}", actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"file_exists={disk_ok}")


async def test_close_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("close_scene", {})
    parsed = _parse(data)
    return TestResult("close_scene", "PASS" if not parsed.get("error") else "FAIL",
                      expected="scene closed", actual=parsed)


async def test_reload_scene(ctx) -> TestResult:
    await ctx.session.call_tool("open_scene", {"scene_path": SCENE_A})
    data = await ctx.session.call_tool("reload_scene", {"scene_path": SCENE_A})
    parsed = _parse(data)
    return TestResult("reload_scene", "PASS" if not parsed.get("error") else "FAIL",
                      expected="scene reloaded", actual=parsed)


async def test_save_all_scenes(ctx) -> TestResult:
    data = await ctx.session.call_tool("save_all_scenes", {})
    parsed = _parse(data)
    count = parsed.get("count", -1)
    return TestResult("save_all_scenes", "PASS" if count >= 0 else "FAIL",
                      expected=f"count >= 0", actual=parsed)


async def test_mark_scene_unsaved(ctx) -> TestResult:
    data = await ctx.session.call_tool("mark_scene_unsaved", {})
    parsed = _parse(data)

    data2 = await ctx.session.call_tool("is_scene_dirty", {})
    parsed2 = _parse(data2)
    return TestResult("mark_scene_unsaved", "PASS",
                      expected="marked unsaved", actual=parsed2)


async def test_branch_to_scene(ctx) -> TestResult:
    branch_path = f"{TEMP_DIR}/branched.tscn"
    data = await ctx.session.call_tool("branch_to_scene", {
        "node_path": "ChildOfA",
        "scene_path": branch_path,
    })
    parsed = _parse(data)
    disk_ok = file_exists(branch_path)
    return TestResult("branch_to_scene", "PASS" if disk_ok else "FAIL",
                      expected=f"path={branch_path}", actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"file_exists={disk_ok}")


async def test_instantiate_scene(ctx) -> TestResult:
    await ctx.session.call_tool("open_scene", {"scene_path": SCENE_A})
    data = await ctx.session.call_tool("instantiate_scene", {
        "scene_path": SCENE_B,
        "parent_path": ".",
        "name": "InstancedB",
    })
    parsed = _parse(data)
    return TestResult("instantiate_scene", "PASS" if not parsed.get("error") else "FAIL",
                      expected="scene instanced", actual=parsed)


async def test_scene_to_branch(ctx) -> TestResult:
    await ctx.session.call_tool("open_scene", {"scene_path": SCENE_A})
    await ctx.session.call_tool("instantiate_scene", {
        "scene_path": SCENE_B,
        "parent_path": ".",
        "name": "ToBranch",
    })
    data = await ctx.session.call_tool("scene_to_branch", {"node_path": "ToBranch"})
    parsed = _parse(data)
    return TestResult("scene_to_branch", "PASS" if parsed.get("converted") else "FAIL",
                      expected="converted=true", actual=parsed)


async def test_rename_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("rename_scene", {
        "source_path": SCENE_A,
        "dest_path": RENAMED,
    })
    parsed = _parse(data)
    old_exists = file_exists(SCENE_A)
    new_exists = file_exists(RENAMED)
    disk_ok = (not old_exists) and new_exists
    return TestResult("rename_scene", "PASS" if disk_ok else "FAIL",
                      expected=f"old gone, new exists", actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"old_exists={old_exists}, new_exists={new_exists}")


async def test_save_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("open_scene", {"scene_path": RENAMED})
    _ = _parse(data)
    data2 = await ctx.session.call_tool("save_scene", {})
    parsed = _parse(data2)
    disk_ok = file_exists(RENAMED)
    return TestResult("save_scene", "PASS" if disk_ok else "FAIL",
                      expected="scene saved", actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"file_exists={disk_ok}")


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
    ToolTest("open_scene", run=test_open_scene, setup=_setup_scenes),
    ToolTest("refresh_filesystem", run=test_refresh_filesystem),
    ToolTest("save_scene_as", run=test_save_scene_as),
    ToolTest("delete_scene", run=test_delete_scene),
    ToolTest("close_scene", run=test_close_scene),
    ToolTest("reload_scene", run=test_reload_scene),
    ToolTest("save_all_scenes", run=test_save_all_scenes),
    ToolTest("mark_scene_unsaved", run=test_mark_scene_unsaved),
    ToolTest("branch_to_scene", run=test_branch_to_scene),
    ToolTest("instantiate_scene", run=test_instantiate_scene),
    ToolTest("scene_to_branch", run=test_scene_to_branch),
    ToolTest("rename_scene", run=test_rename_scene),
    ToolTest("save_scene", run=test_save_scene, teardown=_teardown_all),
]
