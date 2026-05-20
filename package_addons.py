#!/usr/bin/env python3
"""
Godot MCP Addons 打包脚本
编译项目、复制库文件、打包成 zip
"""

import zipfile
import os
import sys
import subprocess
import platform
from pathlib import Path


def run_command(cmd: list, cwd: Path = None) -> bool:
    """运行命令并返回是否成功"""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            check=True,
            capture_output=True,
            text=True
        )
        if result.stdout:
            print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"命令执行失败: {e}", file=sys.stderr)
        if e.stderr:
            print(f"错误输出: {e.stderr}", file=sys.stderr)
        return False


def build_gdext(project_root: Path) -> bool:
    """编译 GDExtension 项目"""
    print("正在编译 GDExtension...")
    
    # 尝试使用 cargo build
    # 优先尝试 --release 模式，失败则尝试 debug
    build_success = run_command(
        ["cargo", "build", "--release", "-p", "godot-mcp-gdext"],
        cwd=project_root
    )
    
    if not build_success:
        print("release 编译失败，尝试 debug 模式...")
        build_success = run_command(
            ["cargo", "build", "-p", "godot-mcp-gdext"],
            cwd=project_root
        )
    
    return build_success


def get_library_filename() -> str:
    """根据平台确定库文件名"""
    system = platform.system()
    if system == "Windows":
        return "godot_mcp_gdext.dll"
    elif system == "Darwin":
        return "libgodot_mcp_gdext.dylib"
    else:
        return "libgodot_mcp_gdext.so"


def copy_library(project_root: Path, source_dir: str) -> bool:
    """复制库文件到插件目录"""
    lib_name = get_library_filename()
    bin_dest = Path(project_root) / source_dir / "godot_mcp" / "bin"
    bin_dest.mkdir(exist_ok=True)
    
    # 先尝试从 release 目录复制
    src = Path(project_root) / "target" / "release" / lib_name
    if not src.exists():
        src = Path(project_root) / "target" / "debug" / lib_name
    
    if not src.exists():
        print(f"错误: 找不到编译好的库文件 {lib_name}", file=sys.stderr)
        return False
    
    dst = bin_dest / lib_name
    import shutil
    shutil.copy2(src, dst)
    print(f"已复制库文件: {lib_name}")
    return True


def package_addons(source_dir: str = "addons", output_file: str = "addons.zip", build: bool = True):
    """
    将 addons 目录打包成 zip 文件
    
    Args:
        source_dir: 源目录路径
        output_file: 输出 zip 文件路径
        build: 是否先编译项目
    """
    project_root = Path(__file__).parent.resolve()
    source_path = project_root / source_dir
    
    if not source_path.exists():
        raise FileNotFoundError(f"源目录不存在: {source_dir}")
    
    # 编译项目
    if build:
        if not build_gdext(project_root):
            raise RuntimeError("编译失败")
        
        if not copy_library(project_root, source_dir):
            raise RuntimeError("复制库文件失败")
    
    # 打包
    print(f"\n正在打包 {source_dir} -> {output_file}...")
    
    with zipfile.ZipFile(project_root / output_file, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(source_path):
            root_path = Path(root)
            for file in files:
                file_path = root_path / file
                arcname = file_path.relative_to(project_root)
                zipf.write(file_path, arcname)
                print(f"  添加: {arcname}")
    
    size_mb = Path(project_root / output_file).stat().st_size / (1024 * 1024)
    print("\nOK 打包完成!")
    print(f"输出文件: {output_file}")
    print(f"文件大小: {size_mb:.2f} MB")


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="打包 Godot MCP 插件")
    parser.add_argument("--no-build", action="store_true", help="跳过编译步骤")
    parser.add_argument("--source", default="addons", help="源目录 (默认: addons)")
    parser.add_argument("--output", default="addons.zip", help="输出文件 (默认: addons.zip)")
    
    args = parser.parse_args()
    
    try:
        package_addons(args.source, args.output, build=not args.no_build)
    except Exception as e:
        print(f"ERROR 打包失败: {e}", file=sys.stderr)
        sys.exit(1)
