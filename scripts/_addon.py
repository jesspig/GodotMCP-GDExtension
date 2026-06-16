"""Godot addon 配置生成与平台库管理。"""

import os
import platform
import shutil
import sys
from pathlib import Path

from scripts._project import PROJECT_ROOT, EXAMPLE_ADDON_DIR, EXAMPLE_BIN_DIR, get_version, get_api_version, BUILD_DIR

PLATFORM_LIBS: dict[str, str] = {
    "linux": "libgodot_mcp_gdext.so",
    "macos": "libgodot_mcp_gdext.dylib",
    "windows": "godot_mcp_gdext.dll",
}


def generate_addon_configs() -> None:
    """在 example/addons/godot_mcp/ 生成 plugin.cfg 和 .gdextension。"""
    version = get_version()
    api_version = get_api_version()

    EXAMPLE_ADDON_DIR.mkdir(parents=True, exist_ok=True)
    EXAMPLE_BIN_DIR.mkdir(parents=True, exist_ok=True)

    (EXAMPLE_ADDON_DIR / "plugin.cfg").write_text(
        '[plugin]\n'
        f'name="Godot MCP"\n'
        f'description="Model Context Protocol bridge for Godot Engine."\n'
        f'author="JessPig"\n'
        f'version="{version}"\n'
        f'script=""\n',
        encoding="utf-8",
    )
    (EXAMPLE_ADDON_DIR / "godot_mcp.gdextension").write_text(
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
    print(f"[CONFIG] Generated addon config files (version={version}, api={api_version})", flush=True)


def _current_platform_key() -> str:
    return {"Windows": "windows", "Linux": "linux", "Darwin": "macos"}.get(platform.system(), "")


def copy_built_libs(config: str) -> int:
    """从构建输出目录复制平台库到 example/addons/godot_mcp/bin/。"""
    lib_name = PLATFORM_LIBS.get(_current_platform_key())
    if not lib_name:
        return 0

    candidates = [
        BUILD_DIR / config / lib_name,
        BUILD_DIR / lib_name,
    ]
    src = next((p for p in candidates if p.exists()), None)
    if not src:
        print(f"[WARN] Built library not found (tried {[str(c) for c in candidates]})", flush=True)
        return 0

    EXAMPLE_BIN_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, EXAMPLE_BIN_DIR / lib_name)
    print(f"[COPY] {lib_name} → example/addons/godot_mcp/bin/", flush=True)
    return 1


def collect_libs(src_dir: Path, dest_dir: Path, recursive: bool = False) -> int:
    """从 src_dir 收集平台库到 dest_dir/addons/godot_mcp/bin/。"""
    bin_dir = dest_dir / "addons" / "godot_mcp" / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    copied = 0
    for expected_name in PLATFORM_LIBS.values():
        if recursive:
            matches = list(src_dir.rglob(expected_name))
            if matches:
                shutil.copy2(matches[0], bin_dir / expected_name)
                print(f"  {expected_name}  ({matches[0].parent})", flush=True)
                copied += 1
        else:
            src = src_dir / expected_name
            if src.exists():
                shutil.copy2(src, bin_dir / expected_name)
                print(f"  {expected_name}", flush=True)
                copied += 1
    return copied
