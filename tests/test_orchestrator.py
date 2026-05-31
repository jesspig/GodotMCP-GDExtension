#!/usr/bin/env python3
"""
GodotMCP Test Orchestrator

Runs all test phases in order, manages Godot editor lifecycle,
and generates a comprehensive test report.

Usage:
    python test_orchestrator.py
    pytest test_orchestrator.py -v --asyncio-mode=auto
"""
import asyncio
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

from dotenv import load_dotenv

from godot_manager import GodotManager
from mcp_client import create_mcp_client, list_tools
from test_context import TestContext
from report import TestReport
from test_phases.base import PhaseReport, PhaseRunner

# Phase modules
from test_phases import (
    phase_01_connect,
    phase_02_net_check,
    phase_03_read_only,
    phase_04_scene_node,
    phase_05_property_2d,
    phase_06_property_3d,
    phase_07_collision,
    phase_08_find,
    phase_09_script,
    phase_10_scene_mgmt,
    phase_11_search,
    phase_12_input,
    phase_13_project_write,
    phase_14_editor_control,
    phase_15_plugin,
    phase_16_undo,
    phase_17_csharp,
    phase_18_cleanup,
)

PHASES = [
    ("connect",       phase_01_connect.TOOL_TESTS),
    ("net_check",     phase_02_net_check.TOOL_TESTS),
    ("read_only",     phase_03_read_only.TOOL_TESTS),
    ("scene_node",    phase_04_scene_node.TOOL_TESTS),
    ("property_2d",   phase_05_property_2d.TOOL_TESTS),
    ("property_3d",   phase_06_property_3d.TOOL_TESTS),
    ("collision",     phase_07_collision.TOOL_TESTS),
    ("find",          phase_08_find.TOOL_TESTS),
    ("script",        phase_09_script.TOOL_TESTS),
    ("scene_mgmt",    phase_10_scene_mgmt.TOOL_TESTS),
    ("search",        phase_11_search.TOOL_TESTS),
    ("input",         phase_12_input.TOOL_TESTS),
    ("project_write", phase_13_project_write.TOOL_TESTS),
    ("editor_control",phase_14_editor_control.TOOL_TESTS),
    ("plugin",        phase_15_plugin.TOOL_TESTS),
    ("undo",          phase_16_undo.TOOL_TESTS),
    ("csharp",        phase_17_csharp.TOOL_TESTS),
    ("cleanup",       phase_18_cleanup.TOOL_TESTS),
]


def load_config() -> dict:
    load_dotenv()

    # Project root is one level up from tests/
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    return {
        "godot_path": os.getenv("GODOT_PATH", ""),
        "godot_headless": os.getenv("GODOT_HEADLESS", "true").lower() == "true",
        "mcp_port": int(os.getenv("GODOT_MCP_HTTP_PORT", "9600")),
        "project_path": os.getenv("GODOT_PROJECT_PATH",
                                   os.path.join(project_root, "example")),
        "project_root": project_root,
        "backup_dir": os.path.join(script_dir, "backup"),
        "output_dir": os.path.join(script_dir, "output"),
    }


def backup_example(cfg: dict) -> dict:
    backup_dir = cfg["backup_dir"]
    example_dir = cfg["project_path"]
    os.makedirs(backup_dir, exist_ok=True)

    status = {}
    for fname in ["icon.svg", ".gitignore", "project.godot"]:
        src = os.path.join(example_dir, fname)
        dst = os.path.join(backup_dir, fname)
        if os.path.isfile(src):
            shutil.copy2(src, dst)
            status[fname] = "backed_up"
        else:
            status[fname] = "not_found"
    return status


def pre_cleanup(cfg: dict):
    example_dir = cfg["project_path"]
    if not os.path.isdir(example_dir):
        return
    for item in os.listdir(example_dir):
        item_path = os.path.join(example_dir, item)
        if item.startswith("test_mcp_") and os.path.isdir(item_path):
            shutil.rmtree(item_path, ignore_errors=True)
        if os.path.isfile(item_path) and (item.endswith(".gd") or item.endswith(".csproj") or item.endswith(".sln")):
            os.remove(item_path)


async def run_test_session(cfg: dict) -> TestReport:
    report = TestReport()
    ctx = TestContext()
    ctx.project_path = cfg["project_path"]

    # --- Phase 00: Pre-clean leftover files ---
    pre_cleanup(cfg)

    # --- Phase 00b: Backup example project ---
    backup_status = backup_example(cfg)
    print(f"[setup] Backup: {backup_status}")

    # --- Phase 00b: Start Godot ---
    manager = GodotManager(
        godot_path=cfg["godot_path"],
        project_path=cfg["project_path"],
        headless=cfg["godot_headless"],
        mcp_port=cfg["mcp_port"],
    )

    started = await manager.ensure_running(timeout=60)
    if not started:
        raise RuntimeError("Failed to start Godot editor or connect to MCP server")

    ctx.godot_process = manager.process
    report.set_env(
        godot_path=cfg["godot_path"],
        headless=cfg["godot_headless"],
        mcp_port=cfg["mcp_port"],
        project_path=cfg["project_path"],
    )

    # --- Connect MCP ---
    mcp_url = f"http://127.0.0.1:{cfg['mcp_port']}/mcp"
    async with create_mcp_client(mcp_url) as session:
        ctx.session = session

        # Discover available tools
        ctx.available_tools = await list_tools(session)
        print(f"[setup] Discovered {len(ctx.available_tools)} tools")

        report.set_env(
            discovered_tools=len(ctx.available_tools),
            tool_list=sorted(list(ctx.available_tools))[:10],
        )

        # --- Run phases ---
        for phase_name, tool_tests in PHASES:
            t0 = time.time()
            phase_report = PhaseReport(name=phase_name)
            phase_report.start_time = t0

            runner = PhaseRunner(ctx, tool_tests)
            results = await runner.run_all()
            phase_report.results = results
            phase_report.end_time = time.time()

            report.add_phase(phase_report)

            passed = phase_report.passed
            failed = phase_report.failed
            skipped = phase_report.skipped
            print(f"[{phase_name}] tested={phase_report.tested} "
                  f"passed={passed} failed={failed} skipped={skipped} "
                  f"({phase_report.duration:.1f}s)")

            if failed > 0:
                for r in results:
                    if r.status == "FAIL":
                        print(f"  FAIL: {r.tool} — {r.error[:120]}")

        # --- Post-run info ---
        report.set_env(
            dotnet_version=ctx.dotnet_version or "not found",
            dotnet_available=ctx.dotnet_available,
        )

        total = report.total_tools
        passed = report.passed
        failed = report.failed
        skipped = report.skipped
        print(f"\n{'='*50}")
        print(f"Total: {total} | Passed: {passed} | Failed: {failed} | Skipped: {skipped}")

    # --- Stop Godot ---
    await manager.stop()

    # --- Restore backup ---
    cleanup_phase = [pr for pr in report.phases if pr.name == "cleanup"]
    if cleanup_phase:
        last = cleanup_phase[-1]
        for r in last.results:
            if r.status == "PASS":
                cleanup_data = r.actual
                report.cleanup_status = (
                    cleanup_data if isinstance(cleanup_data, dict)
                    else {"restored": True}
                )
    else:
        _restore_fallback(cfg)
        report.cleanup_status = {"restored": "fallback"}

    # --- Save reports ---
    json_path = report.save_json(cfg["output_dir"])
    md_path = report.save_markdown(cfg["output_dir"])
    print(f"\nReports saved:")
    print(f"  JSON: {json_path}")
    print(f"  Markdown: {md_path}")

    return report


def _restore_fallback(cfg: dict):
    """Final fallback: restore backed-up files if cleanup phase didn't run."""
    example_dir = cfg["project_path"]
    backup_dir = cfg["backup_dir"]
    for fname in ["icon.svg", ".gitignore", "project.godot"]:
        src = os.path.join(backup_dir, fname)
        dst = os.path.join(example_dir, fname)
        if os.path.isfile(src):
            shutil.copy2(src, dst)


async def main():
    cfg = load_config()

    if not cfg["godot_path"] or not os.path.isfile(cfg["godot_path"]):
        print("Error: GODOT_PATH not set or file not found.")
        print("Create tests/.env with:")
        print('  GODOT_PATH=C:/Path/To/Godot_v4.6-stable_win64.exe')
        print("Or set the GODOT_PATH environment variable.")
        sys.exit(1)

    try:
        report = await run_test_session(cfg)
        if report.failed > 0:
            sys.exit(1)
    except Exception as e:
        print(f"Fatal error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())
