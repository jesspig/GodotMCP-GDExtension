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


def run(cmd: list) -> bool:
    print(f"\n$ {' '.join(cmd)}", flush=True)
    return subprocess.run(cmd, cwd=PROJECT_ROOT).returncode == 0


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
        print(f"\n[CLEAN] Removing {BUILD_DIR}", flush=True)
        shutil.rmtree(BUILD_DIR, ignore_errors=True)

    if not run(["cmake", "-B", str(BUILD_DIR), "-S", str(PROJECT_ROOT)] + cmake_defs):
        sys.exit(1)

    if not run(["cmake", "--build", str(BUILD_DIR), "--config", config]):
        sys.exit(1)

    if not args.no_zip:
        if not run(["cmake", "--build", str(BUILD_DIR), "--config", config, "--target", "package"]):
            sys.exit(1)
    else:
        print("\n[SKIP] addons.zip (--no-zip)")


if __name__ == "__main__":
    main()
