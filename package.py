#!/usr/bin/env python3
"""Package GodotMCP GDExtension libraries into a distributable addons.zip.

Reads version from CMakeLists.txt, generates plugin.cfg / .gdextension,
collects platform-specific GDExtension libraries, and produces a zip
suitable for dropping into a Godot project's addons/ directory.

Usage:
    uv run python package.py                            # pack from local build (example/addons/godot_mcp/bin/)
    uv run python package.py -o dist/addons.zip         # custom output path
    uv run python package.py --libs-dir dist/ --recursive  # collect from CI artifact directory
"""

import argparse
import os
import re
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.resolve()

PLATFORM_LIBS = {
    "linux": "libgodot_mcp_gdext.so",
    "macos": "libgodot_mcp_gdext.dylib",
    "windows": "godot_mcp_gdext.dll",
}


def _read_cmake_var(path: str | Path, var_name: str) -> str | None:
    pat = re.compile(rf'\bset\s*\(\s*{re.escape(var_name)}\s+"([^"]+)"')
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = pat.search(line)
            if m:
                return m.group(1)
    return None


def _get_version() -> str:
    root_cmake = PROJECT_ROOT / "CMakeLists.txt"
    version = _read_cmake_var(root_cmake, "PROJECT_VERSION")
    if not version:
        print("[ERROR] Could not read PROJECT_VERSION from CMakeLists.txt", flush=True)
        sys.exit(1)
    return version


def _generate_addon_configs(target_dir: Path) -> None:
    ext_cmake = PROJECT_ROOT / "extensions" / "CMakeLists.txt"
    version = _get_version()
    api_version = _read_cmake_var(ext_cmake, "GODOTCPP_API_VERSION")
    if not api_version:
        print("[ERROR] Could not read GODOTCPP_API_VERSION from extensions/CMakeLists.txt", flush=True)
        sys.exit(1)

    addon_dir = target_dir / "addons" / "godot_mcp"
    addon_dir.mkdir(parents=True, exist_ok=True)

    (addon_dir / "plugin.cfg").write_text(
        '[plugin]\n'
        f'name="Godot MCP"\n'
        f'description="Model Context Protocol bridge for Godot Engine."\n'
        f'author="JessPig"\n'
        f'version="{version}"\n'
        f'script=""\n',
        encoding="utf-8",
    )
    (addon_dir / "godot_mcp.gdextension").write_text(
        '[configuration]\n'
        '\n'
        'entry_symbol = "gdext_mcp_init"\n'
        f'compatibility_minimum = "{api_version}"\n'
        'reloadable = true\n'
        '\n'
        '[libraries]\n'
        '\n'
        'windows.debug.x86_64   = "res://addons/godot_mcp/bin/godot_mcp_gdext.dll"\n'
        'windows.release.x86_64 = "res://addons/godot_mcp/bin/godot_mcp_gdext.dll"\n'
        'linux.debug.x86_64     = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.so"\n'
        'linux.release.x86_64   = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.so"\n'
        'macos.debug            = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib"\n'
        'macos.release          = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib"\n',
        encoding="utf-8",
    )
    (addon_dir / "bin").mkdir(parents=True, exist_ok=True)
    print(f"[CONFIG] Generated addon config files (version={version}, api={api_version})", flush=True)


def _collect_libs(src_dir: Path, dest_dir: Path) -> int:
    """Copy platform-specific GDExtension libs from src_dir into dest addons/bin/.
    Matches known filenames only (ignores .pdb, .ilk, etc.).
    """
    bin_dir = dest_dir / "addons" / "godot_mcp" / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    copied = 0
    for expected_name in PLATFORM_LIBS.values():
        src = src_dir / expected_name
        if src.exists():
            shutil.copy2(src, bin_dir / expected_name)
            print(f"  {expected_name}", flush=True)
            copied += 1
    return copied


def _collect_libs_recursive(src_dir: Path, dest_dir: Path) -> int:
    """Search src_dir recursively for any platform GDExtension library."""
    bin_dir = dest_dir / "addons" / "godot_mcp" / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    copied = 0
    for expected_name in PLATFORM_LIBS.values():
        matches = list(src_dir.rglob(expected_name))
        if matches:
            shutil.copy2(matches[0], bin_dir / expected_name)
            print(f"  {expected_name}  ({matches[0].parent})", flush=True)
            copied += 1
    return copied


def main():
    parser = argparse.ArgumentParser(description="Package GodotMCP GDExtension into addons.zip")
    parser.add_argument("--libs-dir", type=Path, default=None,
                        help="Directory containing pre-built GDExtension libraries "
                             "(default: example/addons/godot_mcp/bin/)")
    parser.add_argument("--recursive", action="store_true",
                        help="Search libs-dir recursively for platform libs "
                             "(useful when libs are in named subdirectories)")
    parser.add_argument("-o", "--output", type=str, default=None,
                        help="Output zip path (default: addons.zip)")
    args = parser.parse_args()

    version = _get_version()
    output_path = args.output or "addons.zip"

    if args.libs_dir is None:
        libs_dir = PROJECT_ROOT / "example" / "addons" / "godot_mcp" / "bin"
    else:
        libs_dir = args.libs_dir.resolve()

    with tempfile.TemporaryDirectory(prefix="godot-mcp-pkg-") as tmp:
        tmp_dir = Path(tmp)

        _generate_addon_configs(tmp_dir)

        if args.recursive:
            count = _collect_libs_recursive(libs_dir, tmp_dir)
        else:
            count = _collect_libs(libs_dir, tmp_dir)

        if count == 0:
            print(f"[WARN] No GDExtension libraries found in {libs_dir}", flush=True)
            print("  Expected filenames:", ", ".join(PLATFORM_LIBS.values()), flush=True)

        addon_root = tmp_dir / "addons"
        with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for fpath in sorted(addon_root.rglob("*")):
                if fpath.is_file():
                    arcname = fpath.relative_to(tmp_dir)
                    zf.write(fpath, arcname)

        print(f"\n[PACKAGE] {output_path}  ({count} platform libs, "
              f"{os.path.getsize(output_path) / 1024:.0f} KB)", flush=True)


if __name__ == "__main__":
    main()
