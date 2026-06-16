"""YAML 测试发现与异步执行。"""

import asyncio
import os
import sys
from pathlib import Path

from scripts._project import PROJECT_ROOT, TESTS_DIR


def discover_yaml_files(pattern: str | None = None) -> list[str]:
    yaml_dir = TESTS_DIR / "yaml_tests"
    if not yaml_dir.is_dir():
        return []
    if pattern:
        return sorted(str(p) for p in yaml_dir.glob(pattern))
    return sorted(str(p) for p in yaml_dir.glob("*.yaml"))


def run_tests(no_auto: bool = False, file: str | None = None, keep_open: bool = False) -> int:
    sys.path.insert(0, str(TESTS_DIR))

    from dotenv import load_dotenv
    load_dotenv(TESTS_DIR / ".env")

    from test_orchestrator import load_config, run_test_session

    cfg = load_config()

    if not cfg["godot_path"] or not os.path.isfile(cfg["godot_path"]):
        print("Error: GODOT_PATH not set or file not found.")
        print("Create tests/.env with:")
        print('  GODOT_PATH=C:/Path/To/Godot_v4.6-stable_win64.exe')
        return 1

    if file:
        yaml_files = discover_yaml_files(file)
        if not yaml_files:
            print(f"No YAML files matched: {file}")
            return 1
        print(f"[select] {len(yaml_files)} file(s) matched: {[os.path.basename(f) for f in yaml_files]}")

        import test_orchestrator as to
        to.discover_yaml_files = lambda _cfg: yaml_files

    cfg["no_auto"] = no_auto
    cfg["keep_open"] = keep_open

    async def _run():
        try:
            report = await run_test_session(cfg)
            return 1 if report.failed > 0 else 0
        except Exception as e:
            print(f"Fatal error: {e}")
            import traceback
            traceback.print_exc()
            return 1

    return asyncio.run(_run())
