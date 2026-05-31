import os
import shutil

from .base import ToolTest, TestResult



async def test_godot_editor_restart(ctx) -> TestResult:
    registered = "godot_editor_restart" in ctx.available_tools
    return TestResult("godot_editor_restart",
                      "PASS" if registered else "FAIL",
                      expected="tool registered",
                      actual=f"registered={registered}",
                      error="工具会重启编辑器，不做实际调用")


async def test_cleanup_all(ctx) -> TestResult:
    cleanup_log = {}

    try:
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        example_dir = os.path.join(project_root, "example")
        backup_dir = os.path.join(project_root, "tests", "backup")

        # MCP cleanup if Godot is still running
        if ctx.godot_process is not None and ctx.godot_process.poll() is None:
            try:
                await ctx.session.call_tool("close_scene", {})
            except Exception:
                pass
            for f in ctx.created_files:
                try:
                    await ctx.session.call_tool("delete_scene", {"path": f})
                except Exception:
                    pass
            for act in ctx.test_input_actions:
                try:
                    await ctx.session.call_tool("remove_input_action",
                                                 {"name": act})
                except Exception:
                    pass
            for al in ctx.test_autoloads:
                try:
                    await ctx.session.call_tool("remove_autoload",
                                                 {"name": al})
                except Exception:
                    pass
            cleanup_log["mcp_cleanup"] = "completed"

        ctx.godot_process = None

        # Delete any leftover test_mcp_* directories and .gd/.csproj/.sln files
        deleted_dirs = []
        for item in os.listdir(example_dir):
            item_path = os.path.join(example_dir, item)
            if item.startswith("test_mcp_") and os.path.isdir(item_path):
                shutil.rmtree(item_path, ignore_errors=True)
                deleted_dirs.append(item)
        cleanup_log["deleted_test_dirs"] = deleted_dirs

        for fname in os.listdir(example_dir):
            fpath = os.path.join(example_dir, fname)
            if os.path.isfile(fpath) and (fname.endswith(".gd") or fname.endswith(".csproj") or fname.endswith(".sln")):
                os.remove(fpath)
                cleanup_log.setdefault("deleted_files", []).append(fname)

        # Restore backup files
        restored = _restore_backup(example_dir, backup_dir)
        cleanup_log["backup_restored"] = restored

        cleanup_log["status"] = "completed"
    except Exception as e:
        cleanup_log["status"] = f"error: {e}"

    return TestResult("cleanup_all", "PASS",
                      expected="all clean", actual=cleanup_log)


def _restore_backup(example_dir: str, backup_dir: str) -> dict:

    result = {}
    for fname in ["icon.svg", ".gitignore", "project.godot"]:
        src = os.path.join(backup_dir, fname)
        dst = os.path.join(example_dir, fname)
        if os.path.isfile(src):
            shutil.copy2(src, dst)
            result[fname] = "restored"
        else:
            result[fname] = "backup_not_found"
    return result


TOOL_TESTS = [
    ToolTest("godot_editor_restart", run=test_godot_editor_restart),
    ToolTest("cleanup_all", run=test_cleanup_all, always_run=True),
]