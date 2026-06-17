#!/usr/bin/env python3
"""GodotMCP 工具入口。

Usage:
    main.py build [--release] [--clean] [--clean-all] [--purge-cache] [--zip] [-j N]
    main.py package [--libs-dir PATH] [--recursive] [-o OUTPUT]
    main.py test [--no-auto] [--file PATTERN] [--keep-open]
"""

import argparse
import multiprocessing
import shutil
import sys
from pathlib import Path

from scripts._project import PROJECT_ROOT, BUILD_DIR, EXAMPLE_BIN_DIR
from scripts._msvc import ensure_vctargetspath, get_vs_env
from scripts._cmake import configure, build, is_stale_cache_error
from scripts._addon import generate_addon_configs, copy_built_libs, collect_libs, PLATFORM_LIBS
from scripts._archive import create_addons_zip
from scripts._test import run_tests


def _print_build_config():
    cfg_file = BUILD_DIR / "build_config.txt"
    if not cfg_file.exists():
        return
    cfg = {}
    for line in cfg_file.read_text(encoding="utf-8").strip().splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            cfg[k.strip()] = v.strip()
    launcher = cfg.get("compiler_launcher", "")
    launcher = launcher.split("\\")[-1].split("/")[-1] if launcher else "none"
    linker = cfg.get("lld_link", "")
    linker = linker.split("\\")[-1].split("/")[-1] if linker else "default"
    print(f"  Build type:      {cfg.get('build_type', '?')}")
    print(f"  Compiler:        {cfg.get('compiler_id', '?')}")
    print(f"  Generator:       {cfg.get('generator', '?')}")
    print(f"  C++ standard:    {cfg.get('cxx_standard', '?')}")
    print(f"  Launcher:        {launcher}")
    print(f"  Linker:          {linker}")
    lto_mode = cfg.get("lto_mode", "?")
    lto_val = cfg.get("lto", "?")
    print(f"  LTO:             {lto_val}  (mode={lto_mode})")
    print(f"  Unity build:     {cfg.get('unity_enabled', '?')} ({cfg.get('unity_batch_count', '?')} batches, {cfg.get('unity_files', '?')} files)")


def _add_build_parser(sub):
    p = sub.add_parser("build", help="构建 GDExtension 并复制到 example/addons/")
    p.add_argument("--release", action="store_true", help="Release 配置（默认 Debug）")
    p.add_argument("--clean", action="store_true", help="清空 build/ 缓存（保留 _deps/）")
    p.add_argument("--clean-all", action="store_true", help="删除整个 build/ 目录（含 _deps/）")
    p.add_argument("--purge-cache", action="store_true", help="仅清 _deps/，保留 build 缓存")
    p.add_argument("--zip", action="store_true", help="构建完成后额外打包 addons.zip")
    p.add_argument("-j", "--jobs", type=int, default=None, help="并行编译作业数（默认 CPU 核心数）")
    p.add_argument("--lto", choices=["auto", "thin", "full"], default=None,
                   help="LTO 模式：auto (默认)/thin/full")


def _add_package_parser(sub):
    p = sub.add_parser("package", help="打包 example/addons/ 为 addons.zip")
    p.add_argument("--libs-dir", type=Path, default=None,
                   help="预编译库目录（默认 example/addons/godot_mcp/bin/）")
    p.add_argument("--recursive", action="store_true",
                   help="递归搜索 libs-dir（CI 多平台产物）")
    p.add_argument("-o", "--output", type=str, default=None,
                   help="输出路径（默认 addons.zip）")


def _add_test_parser(sub):
    p = sub.add_parser("test", help="运行无头测试流水线")
    p.add_argument("--no-auto", action="store_true", help="不自动启动 Godot")
    p.add_argument("--file", type=str, default=None, help="选定 YAML 文件（glob 模式）")
    p.add_argument("--keep-open", action="store_true", help="测试结束后保持 Godot 运行")


def _cmd_build(args):
    config = "Release" if args.release else "Debug"
    cmake_defs = ["-DCMAKE_BUILD_TYPE=Release"] if args.release else []
    if args.lto:
        cmake_defs.append(f"-DGODOTMCP_LTO_MODE={args.lto}")

    if args.purge_cache:
        deps_dir = BUILD_DIR / "_deps"
        if deps_dir.exists():
            print(f"[PURGE-CACHE] Removing {deps_dir}", flush=True)
            shutil.rmtree(deps_dir, ignore_errors=True)

    if args.clean_all and BUILD_DIR.exists():
        print(f"[CLEAN-ALL] Removing entire build directory", flush=True)
        shutil.rmtree(BUILD_DIR, ignore_errors=True)

    if args.clean and BUILD_DIR.exists():
        print(f"\n[CLEAN] Removing CMake cache in {BUILD_DIR}", flush=True)
        for item in list(BUILD_DIR.iterdir()):
            if item.name == "_deps":
                continue
            if item.is_dir():
                shutil.rmtree(item, ignore_errors=True)
            else:
                item.unlink(missing_ok=True)

    generate_addon_configs()
    ensure_vctargetspath()

    ok, output = configure(cmake_defs)
    if not ok:
        if BUILD_DIR.exists() and is_stale_cache_error(output):
            print("\n[AUTO-CLEAN] Stale build cache detected, reconfiguring...", flush=True)
            shutil.rmtree(BUILD_DIR, ignore_errors=True)
            ok, output = configure(cmake_defs)
        if not ok:
            print("\n[HINT] Try: python main.py build --clean", flush=True)
            sys.exit(1)

    print(f"\n{'=' * 45}")
    print("Build configuration:")
    _print_build_config()
    print(f"{'=' * 45}")

    n_cpus = multiprocessing.cpu_count()
    if args.jobs is not None:
        if args.jobs > n_cpus:
            print(f"[BUILD] -j {args.jobs} exceeds {n_cpus} CPUs, using {n_cpus}", flush=True)
        elif args.jobs < 1:
            print("[BUILD] Error: -j must be >= 1", flush=True)
            sys.exit(1)
        else:
            n_cpus = args.jobs
        print(f"[BUILD] Parallel jobs: {args.jobs}", flush=True)
    else:
        print(f"[BUILD] Parallel jobs: {n_cpus} (auto)", flush=True)

    if not build(config, n_cpus, extra_env=get_vs_env()):
        sys.exit(1)

    copy_built_libs(config)

    if args.zip:
        from scripts._cmake import run
        if not run(["cmake", "--build", str(BUILD_DIR), "--config", config, "--target", "package", "-j", str(n_cpus)]):
            sys.exit(1)


def _cmd_package(args):
    import os as _os
    import tempfile
    import zipfile

    if args.libs_dir is None:
        libs_dir = EXAMPLE_BIN_DIR
    else:
        libs_dir = args.libs_dir.resolve()

    output_path = args.output or "addons.zip"

    with tempfile.TemporaryDirectory(prefix="godot-mcp-pkg-") as tmp:
        tmp_dir = Path(tmp)

        generate_addon_configs()
        configs_dir = PROJECT_ROOT / "example"
        addon_root = tmp_dir / "addons"
        (addon_root / "godot_mcp").mkdir(parents=True, exist_ok=True)
        (addon_root / "godot_mcp" / "bin").mkdir(parents=True, exist_ok=True)

        for name in ("plugin.cfg", "godot_mcp.gdextension"):
            src = configs_dir / "addons" / "godot_mcp" / name
            if src.exists():
                shutil.copy2(src, addon_root / "godot_mcp" / name)

        count = collect_libs(libs_dir, tmp_dir, recursive=args.recursive)
        if count == 0:
            print(f"[WARN] No GDExtension libraries found in {libs_dir}", flush=True)
            print("  Expected filenames:", ", ".join(PLATFORM_LIBS.values()), flush=True)

        with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for fpath in sorted(addon_root.rglob("*")):
                if fpath.is_file():
                    arcname = fpath.relative_to(tmp_dir)
                    zf.write(fpath, arcname)

        print(f"\n[PACKAGE] {output_path}  ({count} platform libs, "
              f"{_os.path.getsize(output_path) / 1024:.0f} KB)", flush=True)


def _cmd_test(args):
    sys.exit(run_tests(no_auto=args.no_auto, file=args.file, keep_open=args.keep_open))


def main():
    parser = argparse.ArgumentParser(
        prog="main.py",
        description="GodotMCP 工具集：构建 / 打包 / 测试",
    )
    sub = parser.add_subparsers(dest="command", required=True)
    _add_build_parser(sub)
    _add_package_parser(sub)
    _add_test_parser(sub)

    args = parser.parse_args()

    if args.command == "build":
        _cmd_build(args)
    elif args.command == "package":
        _cmd_package(args)
    elif args.command == "test":
        _cmd_test(args)


if __name__ == "__main__":
    main()
