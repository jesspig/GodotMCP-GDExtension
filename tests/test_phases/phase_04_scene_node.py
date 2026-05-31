import json

from .base import ToolTest, TestResult
from file_verifier import file_exists, verify_tscn_has_node, get_node_property


TEST_SCENE = "res://test_mcp_node/test_scene.tscn"
TEST_PREFIX = "res://test_mcp_node"


async def test_create_scene(ctx) -> TestResult:
    data = await ctx.session.call_tool("create_scene", {
        "path": TEST_SCENE,
        "root_type": "Node2D",
        "root_name": "TestRoot",
    })
    parsed = _parse(data)
    path = parsed.get("path", "")
    if not path:
        return TestResult("create_scene", "FAIL", expected="path returned",
                          actual=parsed, error="缺少 path 字段")
    ctx.temp_scene_path = path
    ctx.temp_scene_root_name = "TestRoot"
    try:
        await ctx.session.call_tool("refresh_filesystem", {})
        await ctx.session.call_tool("open_scene", {"scene_path": path})
    except Exception as e:
        return TestResult("create_scene", "FAIL", expected="open after create",
                          actual=parsed, error=f"无法打开场景: {e}")
    disk_ok = file_exists(path)
    return TestResult("create_scene", "PASS" if disk_ok else "FAIL",
                      expected=f"path={TEST_SCENE}",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"file_exists={disk_ok}")


async def test_create_node(ctx) -> TestResult:
    data = await ctx.session.call_tool("create_node", {
        "name": "ChildNode",
        "node_type": "Sprite2D",
        "parent_path": ".",
    })
    parsed = _parse(data)
    ctx.created_nodes.append("ChildNode")

    save_data = await ctx.session.call_tool("save_scene", {})
    _ = _parse(save_data)

    disk_ok = verify_tscn_has_node(TEST_SCENE, "ChildNode", "Sprite2D")
    return TestResult("create_node", "PASS" if disk_ok else "FAIL",
                      expected="node created",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"ChildNode/Sprite2D in tscn={disk_ok}")


async def test_get_scene_tree(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_scene_tree", {
        "max_depth": 3,
    })
    parsed = _parse(data)
    return TestResult("get_scene_tree", "PASS" if parsed else "FAIL",
                      expected="scene tree dict",
                      actual=parsed)


async def test_get_node_path(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_path", {
        "node_path": ".",
    })
    parsed = _parse(data)
    return TestResult("get_node_path", "PASS" if parsed else "FAIL",
                      expected="node path string",
                      actual=parsed)


async def test_get_node_info(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_node_info", {"node_path": "."})
    parsed = _parse(data)
    has_name = "name" in parsed
    has_type = "type" in parsed
    has_children = "child_count" in parsed
    ok = has_name and has_type and has_children and not parsed.get("error")
    return TestResult("get_node_info", "PASS" if ok else "FAIL",
                      expected="node info with name, type, child_count",
                      actual=parsed)


async def test_get_property_list(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_property_list", {
        "node_path": ".",
    })
    parsed = _parse(data)
    return TestResult("get_property_list", "PASS" if parsed else "FAIL",
                      expected="property list",
                      actual=parsed)


async def test_get_property(ctx) -> TestResult:
    data = await ctx.session.call_tool("get_property", {
        "node_path": ".",
        "property": "position",
    })
    parsed = _parse(data)
    return TestResult("get_property", "PASS" if parsed else "FAIL",
                      expected="property value",
                      actual=parsed)


async def test_set_property(ctx) -> TestResult:
    data = await ctx.session.call_tool("set_property", {
        "node_path": "ChildNode",
        "property": "position",
        "value": {"x": 100, "y": 200},
    })
    parsed = _parse(data)

    await ctx.session.call_tool("save_scene", {})
    disk_pos = get_node_property(TEST_SCENE, "ChildNode", "position")
    disk_ok = disk_pos is not None

    data2 = await ctx.session.call_tool("get_property", {
        "node_path": "ChildNode",
        "property": "position",
    })
    parsed2 = _parse(data2)
    return TestResult("set_property",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="position set",
                      actual=parsed2,
                      disk_verified=disk_ok,
                      disk_detail=f"position in tscn={disk_pos}")


async def test_rename_node(ctx) -> TestResult:
    data = await ctx.session.call_tool("rename_node", {
        "node_path": "ChildNode",
        "new_name": "RenamedNode",
    })
    parsed = _parse(data)
    if not parsed.get("error"):
        if "ChildNode" in ctx.created_nodes:
            ctx.created_nodes.remove("ChildNode")
        ctx.created_nodes.append("RenamedNode")

    await ctx.session.call_tool("save_scene", {})
    old_ok = verify_tscn_has_node(TEST_SCENE, "ChildNode")
    new_ok = verify_tscn_has_node(TEST_SCENE, "RenamedNode")
    disk_ok = (not old_ok) and new_ok
    return TestResult("rename_node", "PASS" if disk_ok else "FAIL",
                      expected="old=gone, new=exists",
                      actual=parsed,
                      disk_verified=disk_ok,
                      disk_detail=f"old_exists={old_ok} new_exists={new_ok}")


async def test_duplicate_node(ctx) -> TestResult:
    data = await ctx.session.call_tool("duplicate_node", {
        "node_path": "RenamedNode",
    })
    parsed = _parse(data)

    await ctx.session.call_tool("save_scene", {})

    _, _, dup_name = _find_duplicate_name(parsed, "RenamedNode")
    if dup_name:
        ctx.created_nodes.append(dup_name)

    return TestResult("duplicate_node", "PASS" if not parsed.get("error") else "FAIL",
                      expected="node duplicated",
                      actual=parsed,
                      disk_verified=False)


async def test_move_node(ctx) -> TestResult:
    data = await ctx.session.call_tool("create_node", {
        "name": "ContainerNode",
        "node_type": "Node2D",
        "parent_path": ".",
    })
    _parse(data)
    ctx.created_nodes.append("ContainerNode")

    data2 = await ctx.session.call_tool("move_node", {
        "node_path": "RenamedNode",
        "new_parent_path": "ContainerNode",
    })
    parsed = _parse(data2)

    await ctx.session.call_tool("save_scene", {})

    return TestResult("move_node", "PASS" if not parsed.get("error") else "FAIL",
                      expected="node moved",
                      actual=parsed,
                      disk_verified=False)


async def test_reset_parent(ctx) -> TestResult:
    data = await ctx.session.call_tool("reset_parent", {
        "node_path": "RenamedNode",
        "new_parent_path": ".",
    })
    parsed = _parse(data)

    await ctx.session.call_tool("save_scene", {})

    return TestResult("reset_parent", "PASS" if not parsed.get("error") else "FAIL",
                      expected="node reparented",
                      actual=parsed,
                      disk_verified=False)


async def test_set_as_root(ctx) -> TestResult:
    data = await ctx.session.call_tool("create_node", {
        "name": "NewRoot",
        "node_type": "Node2D",
        "parent_path": ".",
    })
    _parse(data)
    ctx.created_nodes.append("NewRoot")

    data2 = await ctx.session.call_tool("set_as_root", {
        "node_path": "NewRoot",
    })
    parsed = _parse(data2)
    return TestResult("set_as_root", "PASS" if not parsed.get("error") else "FAIL",
                      expected="node set as root",
                      actual=parsed,
                      disk_verified=False)


async def test_add_node_to_group(ctx) -> TestResult:
    data = await ctx.session.call_tool("add_node_to_group", {
        "node_path": "RenamedNode",
        "group": "test_group_mcp",
    })
    parsed = _parse(data)
    return TestResult("add_node_to_group", "PASS" if not parsed.get("error") else "FAIL",
                      expected="added to group",
                      actual=parsed,
                      disk_verified=False)


async def test_remove_node_from_group(ctx) -> TestResult:
    data = await ctx.session.call_tool("remove_node_from_group", {
        "node_path": "RenamedNode",
        "group": "test_group_mcp",
    })
    parsed = _parse(data)
    return TestResult("remove_node_from_group",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="removed from group",
                      actual=parsed,
                      disk_verified=False)


async def test_delete_node(ctx) -> TestResult:
    data = await ctx.session.call_tool("delete_node", {
        "node_path": "ContainerNode",
    })
    parsed = _parse(data)
    if "ContainerNode" in ctx.created_nodes:
        ctx.created_nodes.remove("ContainerNode")
    return TestResult("delete_node", "PASS" if not parsed.get("error") else "FAIL",
                      expected="node deleted",
                      actual=parsed,
                      disk_verified=False)


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


def _find_duplicate_name(parsed, original_name):
    for k, v in parsed.items():
        if isinstance(v, str) and "dup" in v.lower() or "copy" in v.lower():
            return k, v, v
    return None, None, f"{original_name}2"
TOOL_TESTS = [
    ToolTest("create_scene", run=test_create_scene),
    ToolTest("create_node", run=test_create_node),
    ToolTest("get_scene_tree", run=test_get_scene_tree),
    ToolTest("get_node_path", run=test_get_node_path),
    ToolTest("get_node_info", run=test_get_node_info),
    ToolTest("get_property_list", run=test_get_property_list),
    ToolTest("get_property", run=test_get_property),
    ToolTest("set_property", run=test_set_property),
    ToolTest("rename_node", run=test_rename_node),
    ToolTest("duplicate_node", run=test_duplicate_node),
    ToolTest("move_node", run=test_move_node),
    ToolTest("reset_parent", run=test_reset_parent),
    ToolTest("set_as_root", run=test_set_as_root),
    ToolTest("add_node_to_group", run=test_add_node_to_group),
    ToolTest("remove_node_from_group", run=test_remove_node_from_group),
    ToolTest("delete_node", run=test_delete_node),
]
