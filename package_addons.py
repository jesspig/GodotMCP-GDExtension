#!/usr/bin/env python3
"""
Godot MCP 一键构建&打包脚本

流程：
  1. 从 Cargo.toml 读取版本号并更新 plugin.cfg
  2. 编译 godot-mcp-gdext（GDExtension 动态链接库）
  3. 编译 godot-mcp-server（MCP 服务端，确保最新但不打包到 zip）
  4. 复制编译好的库文件到 addons/godot_mcp/bin/
  5. 打包 addons/ 为 addons.zip
"""

import re
import zipfile
import os
import sys
import subprocess
import shutil
import platform
from pathlib import Path


def run_command(cmd: list, cwd: Path) -> bool:
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            check=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        if result.stdout:
            print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"命令执行失败: {e}", file=sys.stderr)
        if e.stderr:
            print(f"错误输出: {e.stderr}", file=sys.stderr)
        return False


def get_library_filename() -> str:
    system = platform.system()
    if system == "Windows":
        return "godot_mcp_gdext.dll"
    elif system == "Darwin":
        return "libgodot_mcp_gdext.dylib"
    else:
        return "libgodot_mcp_gdext.so"


def read_workspace_version(project_root: Path) -> str:
    cargo_toml = project_root / "Cargo.toml"
    content = cargo_toml.read_text(encoding="utf-8")
    m = re.search(r'^\[workspace\.package\]\s*\nversion\s*=\s*"([^"]+)"', content, re.MULTILINE)
    if not m:
        print("错误: 无法从 Cargo.toml 读取版本号", file=sys.stderr)
        sys.exit(1)
    return m.group(1)


def update_plugin_version(project_root: Path, version: str) -> bool:
    plugin_cfg = project_root / "addons" / "godot_mcp" / "plugin.cfg"
    content = plugin_cfg.read_text(encoding="utf-8")
    new_content = re.sub(
        r'^version="[^"]*"',
        f'version="{version}"',
        content,
        flags=re.MULTILINE,
    )
    if new_content == content:
        print(f"[OK] plugin.cfg 版本已为 {version}，无需更新")
        return True
    plugin_cfg.write_text(new_content, encoding="utf-8")
    print(f"[OK] plugin.cfg 版本已更新为 {version}")
    return True


def build_all(project_root: Path) -> bool:
    print("=" * 50)
    print("[1/4] 编译 GDExtension (godot-mcp-gdext)")
    if not run_command(["cargo", "build", "-p", "godot-mcp-gdext"], cwd=project_root):
        return False

    print("[2/4] 编译 MCP Server (godot-mcp-server)")
    if not run_command(["cargo", "build", "-p", "godot-mcp-server"], cwd=project_root):
        return False

    return True


def copy_library(project_root: Path) -> bool:
    lib_name = get_library_filename()
    src = project_root / "target" / "debug" / lib_name

    if not src.exists():
        print(f"错误: 找不到编译产物 {src}", file=sys.stderr)
        return False

    dest = project_root / "addons" / "godot_mcp" / "bin"
    dest.mkdir(exist_ok=True)
    shutil.copy2(src, dest / lib_name)
    print(f"[OK] 已复制: {lib_name}")

    return True


def package_addons(project_root: Path, output_file: str) -> bool:
    source_path = project_root / "addons"

    if not source_path.exists():
        print(f"错误: addons 目录不存在", file=sys.stderr)
        return False

    print("[3/4] 打包 addons -> addons.zip")

    output_path = project_root / output_file
    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(source_path):
            root_path = Path(root)
            for file in files:
                file_path = root_path / file
                arcname = file_path.relative_to(project_root)
                zipf.write(file_path, arcname)
                print(f"  添加: {arcname}")

    size_mb = output_path.stat().st_size / (1024 * 1024)
    print(f"\n{'=' * 50}")
    print(f"打包完成!")
    print(f"输出文件: {output_file}")
    print(f"文件大小: {size_mb:.2f} MB")
    return True


def main():
    project_root = Path(__file__).parent.resolve()

    print("[0/4] 同步版本号到 plugin.cfg")
    version = read_workspace_version(project_root)
    update_plugin_version(project_root, version)

    if not build_all(project_root):
        print("编译失败，终止", file=sys.stderr)
        sys.exit(1)

    if not copy_library(project_root):
        print("复制库文件失败，终止", file=sys.stderr)
        sys.exit(1)

    if not package_addons(project_root, "addons.zip"):
        sys.exit(1)


if __name__ == "__main__":
    main()