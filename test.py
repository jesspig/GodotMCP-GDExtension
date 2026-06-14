#!/usr/bin/env python3
"""Thin wrapper around the YAML test orchestrator.

Usage:
    uv run python test.py                          # run all tests (auto-start Godot)
    uv run python test.py --no-auto                # skip Godot auto-start (must be running)
    uv run python test.py --file 03_*.yaml          # run specific YAML files (glob)
    uv run python test.py --keep-open              # keep Godot running after tests
"""

import argparse
import asyncio
import glob
import os
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.resolve()
TESTS_DIR = PROJECT_ROOT / "tests"


def discover_yaml_files(pattern: str | None) -> list[str]:
    yaml_dir = TESTS_DIR / "yaml_tests"
    if not yaml_dir.is_dir():
        return []
    if pattern:
        return sorted(str(p) for p in yaml_dir.glob(pattern))
    return sorted(str(p) for p in yaml_dir.glob("*.yaml"))


async def main():
    parser = argparse.ArgumentParser(description="Run GodotMCP YAML tests")
    parser.add_argument("--no-auto", action="store_true",
                        help="Skip Godot auto-start (assume already running)")
    parser.add_argument("--file", type=str, default=None,
                        help="Glob pattern to select specific YAML files (e.g. '03_*.yaml')")
    parser.add_argument("--keep-open", action="store_true",
                        help="Keep Godot running after tests complete")
    args = parser.parse_args()

    sys.path.insert(0, str(TESTS_DIR))

    from dotenv import load_dotenv
    load_dotenv(TESTS_DIR / ".env")

    from test_orchestrator import load_config, run_test_session
    from godot_manager import GodotManager

    cfg = load_config()

    if not cfg["godot_path"] or not os.path.isfile(cfg["godot_path"]):
        print("Error: GODOT_PATH not set or file not found.")
        print("Create tests/.env with:")
        print('  GODOT_PATH=C:/Path/To/Godot_v4.6-stable_win64.exe')
        sys.exit(1)

    if args.file:
        yaml_files = discover_yaml_files(args.file)
        if not yaml_files:
            print(f"No YAML files matched: {args.file}")
            sys.exit(1)
        print(f"[select] {len(yaml_files)} file(s) matched: {[os.path.basename(f) for f in yaml_files]}")

        import test_orchestrator as to
        to.discover_yaml_files = lambda _cfg: yaml_files

    cfg["no_auto"] = args.no_auto
    cfg["keep_open"] = args.keep_open

    try:
        report = await run_test_session(cfg)
    except Exception as e:
        print(f"Fatal error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

    if report.failed > 0:
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())
