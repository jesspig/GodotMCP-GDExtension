#!/usr/bin/env python3
"""Thin wrapper around CMake + CPack for building and packaging GodotMCP.

Usage:
    py -3 build.py                # debug build + addons.zip
    py -3 build.py --release      # release build + addons.zip
    py -3 build.py --no-zip       # build only, skip zip
    py -3 build.py --no-server    # skip building godot-mcp-server
    py -3 build.py --clean        # wipe build cache before building
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.resolve()
BUILD_DIR = PROJECT_ROOT / "build"

# Error patterns that indicate a stale CMake cache and need a clean reconfigure
STALE_CACHE_PATTERNS = [
    "MSB4019",          # .NET SDK path mismatch (Microsoft.Cpp.Default.props)
    "VCTargetsPath",    # VC++ targets path resolution failure
    "CMAKE_C_COMPILER", # Compiler path changed
    "CMAKE_CXX_COMPILER",
]


def detect_vctargetspath() -> str | None:
    """Detect VCTargetsPath by finding Visual Studio's MSBuild VC directory."""
    try:
        result = subprocess.run(
            ["vswhere", "-latest", "-property", "installationPath"],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        if result.returncode == 0 and result.stdout.strip():
            vs_path = Path(result.stdout.strip())
            # Try v180 (VS 2026), v170 (VS 2022), v160 (VS 2019)
            for ver in ("v180", "v170", "v160"):
                vc_path = vs_path / "MSBuild" / "Microsoft" / "VC" / ver
                if vc_path.exists():
                    return str(vc_path) + "\\"
    except FileNotFoundError:
        pass
    return None


def run(cmd: list, capture: bool = False) -> tuple[bool, str]:
    print(f"\n$ {' '.join(cmd)}", flush=True)
    env = {**subprocess.os.environ, "PYTHONIOENCODING": "utf-8"}
    if capture:
        result = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", env=env)
        output = (result.stdout or "") + (result.stderr or "")
        if result.returncode != 0:
            print(output, flush=True)
        return result.returncode == 0, output
    else:
        return subprocess.run(cmd, cwd=PROJECT_ROOT, env=env).returncode == 0, ""


def is_stale_cache_error(output: str) -> bool:
    """Check if CMake output indicates a stale build cache."""
    return any(pat in output for pat in STALE_CACHE_PATTERNS)


def configure(extra_defs: list, force_capture: bool = False) -> tuple[bool, str]:
    """Run CMake configure. Returns (success, output)."""
    cmd = ["cmake", "-B", str(BUILD_DIR), "-S", str(PROJECT_ROOT)] + extra_defs
    if force_capture:
        return run(cmd, capture=True)
    # First attempt: stream output directly
    ok, _ = run(cmd)
    if ok:
        return True, ""
    # Second attempt: capture output to check for stale cache
    return run(cmd, capture=True)


def main():
    parser = argparse.ArgumentParser(description="Build and package GodotMCP via CMake")
    parser.add_argument("--release", action="store_true", help="Build with Release configuration")
    parser.add_argument("--clean", action="store_true", help="Wipe build/ directory before configuring")
    parser.add_argument("--no-zip", action="store_true", help="Skip producing addons.zip")
    parser.add_argument("--no-server", action="store_true", help="Skip building godot-mcp-server")
    args = parser.parse_args()

    config = "Release" if args.release else "Debug"
    cmake_defs = []
    if args.release:
        cmake_defs += ["-DRELEASE=ON"]
    if args.no_server:
        cmake_defs += ["-DNO_SERVER=ON"]

    if args.clean and BUILD_DIR.exists():
        print(f"\n[CLEAN] Removing CMake cache in {BUILD_DIR}", flush=True)
        # Only remove CMake cache files, preserve _deps (FetchContent cache)
        for item in BUILD_DIR.iterdir():
            if item.name == "_deps":
                continue  # Preserve FetchContent cache (godot-cpp)
            if item.is_dir():
                shutil.rmtree(item, ignore_errors=True)
            else:
                item.unlink(missing_ok=True)

    # --- Ensure VCTargetsPath is set (fixes .NET SDK preview interference) ---
    if "VCTargetsPath" not in subprocess.os.environ:
        vc_path = detect_vctargetspath()
        if vc_path:
            print(f"\n[ENV] VCTargetsPath={vc_path}", flush=True)
            subprocess.os.environ["VCTargetsPath"] = vc_path

    # --- Configure ---
    ok, output = configure(cmake_defs, force_capture=args.clean is False and BUILD_DIR.exists())
    if not ok:
        # Auto-retry with clean if stale cache detected
        if BUILD_DIR.exists() and is_stale_cache_error(output):
            print("\n[AUTO-CLEAN] Stale build cache detected, reconfiguring...", flush=True)
            shutil.rmtree(BUILD_DIR, ignore_errors=True)
            ok, output = configure(cmake_defs)
        if not ok:
            print("\n[HINT] Try: py -3 build.py --clean", flush=True)
            sys.exit(1)

    # --- Build ---
    if not run(["cmake", "--build", str(BUILD_DIR), "--config", config]):
        sys.exit(1)

    # --- Package ---
    if not args.no_zip:
        if not run(["cmake", "--build", str(BUILD_DIR), "--config", config, "--target", "package"]):
            sys.exit(1)
    else:
        print("\n[SKIP] addons.zip (--no-zip)")


if __name__ == "__main__":
    main()
