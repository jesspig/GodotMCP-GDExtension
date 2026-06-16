import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = PROJECT_ROOT / "build"

EXAMPLE_ADDON_DIR = PROJECT_ROOT / "example" / "addons" / "godot_mcp"
EXAMPLE_BIN_DIR = EXAMPLE_ADDON_DIR / "bin"

TESTS_DIR = PROJECT_ROOT / "tests"


def _read_cmake_var(path: str | Path, var_name: str) -> str | None:
    pat = re.compile(rf'\bset\s*\(\s*{re.escape(var_name)}\s+"([^"]+)"')
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = pat.search(line)
            if m:
                return m.group(1)
    return None


def get_version() -> str:
    version = _read_cmake_var(PROJECT_ROOT / "CMakeLists.txt", "PROJECT_VERSION")
    if not version:
        print("[ERROR] Could not read PROJECT_VERSION from CMakeLists.txt", flush=True)
        sys.exit(1)
    return version


def get_api_version() -> str:
    api = _read_cmake_var(PROJECT_ROOT / "extensions" / "CMakeLists.txt", "GODOTCPP_API_VERSION")
    if not api:
        print("[ERROR] Could not read GODOTCPP_API_VERSION from extensions/CMakeLists.txt", flush=True)
        sys.exit(1)
    return api
