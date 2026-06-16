#!/usr/bin/env python3
"""
GodotMCP Test Orchestrator

Runs YAML test files through the C++ /run-tests endpoint,
manages Godot editor lifecycle, and generates a test report.

Usage:
    python test_orchestrator.py
    pytest test_orchestrator.py -v --asyncio-mode=auto
"""
import asyncio
import glob
import os
import sys
import time

from dotenv import load_dotenv

import httpx

from godot_manager import GodotManager
from report import TestReport
from test_phases.base import PhaseReport, TestResult


def discover_yaml_files(cfg: dict) -> list[str]:
    """Find .test.yaml or .yaml test files in yaml_tests/ directory."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    yaml_dir = os.path.join(script_dir, "yaml_tests")
    if not os.path.isdir(yaml_dir):
        return []
    files = []
    for fname in sorted(os.listdir(yaml_dir)):
        if fname.endswith((".yaml", ".yml")):
            files.append(os.path.join(yaml_dir, fname))
    return files


async def check_run_tests_endpoint(mcp_port: int) -> bool:
    """Check if the C++ /run-tests endpoint is available."""
    try:
        async with httpx.AsyncClient() as client:
            resp = await client.post(
                f"http://127.0.0.1:{mcp_port}/run-tests",
                content="name: ping\ntests: []",
                headers={"Content-Type": "application/x-yaml"},
                timeout=5,
            )
            return resp.status_code == 200
    except Exception:
        return False


async def run_yaml_test_file(file_path: str, mcp_port: int) -> dict:
    """Read a YAML file and POST it to /run-tests. Returns the JSON result."""
    with open(file_path, "r", encoding="utf-8") as f:
        yaml_content = f.read()
    async with httpx.AsyncClient() as client:
        resp = await client.post(
            f"http://127.0.0.1:{mcp_port}/run-tests",
            content=yaml_content,
            headers={"Content-Type": "application/x-yaml"},
            timeout=120,
        )
        resp.raise_for_status()
        return resp.json()


def load_config() -> dict:
    load_dotenv()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    return {
        "godot_path": os.getenv("GODOT_PATH", ""),
        "godot_headless": os.getenv("GODOT_HEADLESS", "true").lower() == "true",
        "mcp_port": int(os.getenv("GODOT_MCP_HTTP_PORT", "9600")),
        "project_path": os.getenv("GODOT_PROJECT_PATH",
                                   os.path.join(project_root, "example")),
        "project_root": project_root,
        "output_dir": os.path.join(script_dir, "output"),
    }


def pre_cleanup(cfg: dict):
    example_dir = cfg["project_path"]
    if not os.path.isdir(example_dir):
        return
    for item in os.listdir(example_dir):
        item_path = os.path.join(example_dir, item)
        if item.startswith("test_mcp_") and os.path.isdir(item_path):
            import shutil
            shutil.rmtree(item_path, ignore_errors=True)


def cleanup_old_reports(cfg: dict, keep_count: int = 10):
    output_dir = cfg.get("output_dir", "")
    if not output_dir or not os.path.isdir(output_dir):
        return
    pattern = os.path.join(output_dir, "report-*.json")
    files = sorted(glob.glob(pattern), key=os.path.getmtime)
    if len(files) <= keep_count:
        return
    import shutil
    for f in files[:-keep_count]:
        try:
            os.remove(f)
        except OSError:
            pass


async def run_test_session(cfg: dict) -> TestReport:
    report = TestReport()

    # --- Phase 00: Pre-clean leftover files and old reports ---
    pre_cleanup(cfg)
    cleanup_old_reports(cfg)

    # --- Start Godot ---
    manager = GodotManager(
        godot_path=cfg["godot_path"],
        project_path=cfg["project_path"],
        headless=cfg["godot_headless"],
        mcp_port=cfg["mcp_port"],
    )

    try:
        if cfg.get("no_auto"):
            if not await manager._check_mcp_ready():
                raise RuntimeError("MCP server not reachable on port " + str(cfg["mcp_port"]))
            print("[setup] Skipping Godot auto-start (--no-auto)")
        else:
            started = await manager.ensure_running(timeout=60)
            if not started:
                raise RuntimeError("Failed to start Godot editor or connect to MCP server")

        report.set_env(
            godot_path=cfg["godot_path"],
            headless=cfg["godot_headless"],
            mcp_port=cfg["mcp_port"],
            project_path=cfg["project_path"],
        )
    except:
        await manager.stop()
        raise

    # --- Check for C++ /run-tests endpoint ---
    try:
        use_cpp_engine = await check_run_tests_endpoint(cfg["mcp_port"])
        if use_cpp_engine:
            print("[setup] C++ TestEngine available 鈥?using /run-tests endpoint")

            yaml_files = discover_yaml_files(cfg)
            if not yaml_files:
                print("[setup] No YAML test files found in yaml_tests/")
            else:
                for yaml_path in yaml_files:
                    t0 = time.time()
                    fname = os.path.basename(yaml_path)
                    try:
                        result = await run_yaml_test_file(yaml_path, cfg["mcp_port"])
                        elapsed = time.time() - t0

                        summary = result.get("summary", {})
                        total = summary.get("total", 0)
                        passed = summary.get("passed", 0)
                        failed = summary.get("failed", 0)
                        deleted = summary.get("cleanup_deleted", [])
                        skipped = summary.get("cleanup_skipped", [])

                        phase_report = PhaseReport(name=fname.replace(".yaml", "").replace(".yml", ""))
                        phase_report.start_time = t0
                        for t in result.get("tests", []):
                            tr = TestResult(
                                tool=t.get("tool", ""),
                                status="PASS" if t.get("passed") else "FAIL",
                                expected=t.get("description", ""),
                                actual={"raw": t},
                                error=t.get("error", ""),
                            )
                            phase_report.results.append(tr)
                        phase_report.end_time = time.time()
                        report.add_phase(phase_report)

                        status = "OK" if failed == 0 else f"FAIL({failed})"
                        print(f"[{fname}] {total} tests, {passed} passed, {failed} failed "
                              f"({elapsed:.1f}s) | cleanup: {len(deleted)} del, {len(skipped)} skip")
                        if failed > 0:
                            for t in result.get("tests", []):
                                if not t.get("passed"):
                                    print(f"  FAIL: {t.get('tool')} ({t.get('description', '')}) 鈥?{t.get('error', '')[:120]}")
                    except Exception as e:
                        elapsed = time.time() - t0
                        phase_report = PhaseReport(name=fname.replace(".yaml", "").replace(".yml", ""))
                        phase_report.start_time = t0
                        phase_report.end_time = time.time()
                        report.add_phase(phase_report)
                        print(f"[{fname}] ERROR ({elapsed:.1f}s): {e}")

            total = report.total_tools
            passed = report.passed
            failed = report.failed
            print(f"\n{'='*50}")
            print(f"Total: {total} | Passed: {passed} | Failed: {failed}")

            report.set_env(
                engine="cpp",
                yaml_files=len(yaml_files),
            )
        else:
            raise RuntimeError("C++ /run-tests endpoint not available 鈥?cannot run tests")

    finally:
        if not cfg.get("keep_open"):
            await manager.stop()

    # --- Save reports ---
    json_path = report.save_json(cfg["output_dir"])
    md_path = report.save_markdown(cfg["output_dir"])
    print(f"\nReports saved:")
    print(f"  JSON: {json_path}")
    print(f"  Markdown: {md_path}")

    return report


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
