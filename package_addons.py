#!/usr/bin/env python3
"""Godot MCP build and packaging script.

Default flow (no flags):
  1. Sync version from Cargo.toml to plugin.cfg
  2. Kill any running godot-mcp-server.exe so cargo can overwrite the binary
  3. Build BOTH crates: godot-mcp-gdext + godot-mcp-server
  4. Copy fresh GDExtension shared library to addons/godot_mcp/bin/
  5. Zip addons/ into addons.zip
  6. Print a summary of every produced artifact (path, size, mtime)

Flags:
  --release      Build with --release profile (target/release/* instead of debug)
  --clean        Run `cargo clean` and wipe addons/godot_mcp/bin/ before building
  --no-zip       Skip the final addons.zip step (faster iteration)
  --no-server    Skip building godot-mcp-server (e.g. when only the dll changed)
"""

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
import time
import zipfile
from pathlib import Path


SERVER_BIN = "godot-mcp-server.exe" if platform.system() == "Windows" else "godot-mcp-server"


def stream_command(cmd: list, cwd: Path) -> bool:
    """Run a subprocess and stream its stdout/stderr live so the user can see progress."""
    print(f"$ {' '.join(cmd)}", flush=True)
    try:
        proc = subprocess.run(cmd, cwd=cwd, check=False)
    except FileNotFoundError as e:
        print(f"[ERROR] command not found: {e}", file=sys.stderr)
        return False
    if proc.returncode != 0:
        print(f"[ERROR] command failed with exit code {proc.returncode}", file=sys.stderr)
        return False
    return True


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
    m = re.search(
        r'^\[workspace\.package\]\s*\nversion\s*=\s*"([^"]+)"', content, re.MULTILINE
    )
    if not m:
        print("[ERROR] cannot read version from Cargo.toml", file=sys.stderr)
        sys.exit(1)
    return m.group(1)


def update_plugin_version(project_root: Path, version: str) -> None:
    plugin_cfg = project_root / "addons" / "godot_mcp" / "plugin.cfg"
    content = plugin_cfg.read_text(encoding="utf-8")
    new_content = re.sub(
        r'^version="[^"]*"', f'version="{version}"', content, flags=re.MULTILINE
    )
    if new_content == content:
        print(f"[OK] plugin.cfg version already {version}")
        return
    plugin_cfg.write_text(new_content, encoding="utf-8")
    print(f"[OK] plugin.cfg version updated to {version}")


def kill_server_processes() -> None:
    """Kill any running godot-mcp-server processes; otherwise cargo cannot overwrite the .exe."""
    system = platform.system()
    if system == "Windows":
        # taskkill returns non-zero when no matching process; that's fine.
        subprocess.run(
            ["taskkill", "/F", "/IM", SERVER_BIN, "/T"],
            capture_output=True,
            text=True,
        )
    else:
        subprocess.run(["pkill", "-f", SERVER_BIN], capture_output=True, text=True)
    time.sleep(0.5)


def clean_artifacts(project_root: Path) -> bool:
    print("\n[CLEAN] removing cargo target/ and addons/godot_mcp/bin/")
    if not stream_command(["cargo", "clean"], cwd=project_root):
        return False
    bin_dir = project_root / "addons" / "godot_mcp" / "bin"
    if bin_dir.exists():
        shutil.rmtree(bin_dir)
        print(f"[OK] removed {bin_dir}")
    return True


def build_crates(project_root: Path, release: bool, skip_server: bool) -> bool:
    profile_flag = ["--release"] if release else []
    print("\n[BUILD] godot-mcp-gdext")
    if not stream_command(
        ["cargo", "build", *profile_flag, "-p", "godot-mcp-gdext"], cwd=project_root
    ):
        return False

    if skip_server:
        print("[SKIP] godot-mcp-server (--no-server)")
        return True

    print("\n[BUILD] godot-mcp-server")
    if not stream_command(
        ["cargo", "build", *profile_flag, "-p", "godot-mcp-server"], cwd=project_root
    ):
        return False
    return True


def copy_library(project_root: Path, release: bool) -> bool:
    lib_name = get_library_filename()
    profile_dir = "release" if release else "debug"
    src = project_root / "target" / profile_dir / lib_name
    if not src.exists():
        print(f"[ERROR] build artifact not found: {src}", file=sys.stderr)
        return False
    dest_dir = project_root / "addons" / "godot_mcp" / "bin"
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest = dest_dir / lib_name
    shutil.copy2(src, dest)
    print(f"\n[COPY] {src.name} -> {dest.relative_to(project_root)}")
    return True


def package_addons(project_root: Path, output_file: str) -> bool:
    source_path = project_root / "addons"
    if not source_path.exists():
        print("[ERROR] addons directory does not exist", file=sys.stderr)
        return False

    print(f"\n[ZIP] addons/ -> {output_file}")
    output_path = project_root / output_file
    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        for root, _dirs, files in os.walk(source_path):
            root_path = Path(root)
            for file in files:
                file_path = root_path / file
                arcname = file_path.relative_to(project_root)
                zipf.write(file_path, arcname)
    size_mb = output_path.stat().st_size / (1024 * 1024)
    print(f"[OK] {output_file} ({size_mb:.2f} MB)")
    return True


def print_summary(project_root: Path, release: bool, packaged_zip: bool) -> None:
    profile_dir = "release" if release else "debug"
    lib_name = get_library_filename()

    paths = [
        project_root / "target" / profile_dir / lib_name,
        project_root / "target" / profile_dir / SERVER_BIN,
        project_root / "addons" / "godot_mcp" / "bin" / lib_name,
    ]
    if packaged_zip:
        paths.append(project_root / "addons.zip")

    print("\n" + "=" * 70)
    print(f"BUILD SUMMARY (profile = {profile_dir})")
    print("=" * 70)
    print(f"{'Artifact':<60} {'Size':>10}  Modified")
    print("-" * 70)
    for p in paths:
        if p.exists():
            stat = p.stat()
            size = f"{stat.st_size:,} B"
            mtime = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(stat.st_mtime))
            rel = p.relative_to(project_root)
            print(f"{str(rel):<60} {size:>10}  {mtime}")
        else:
            print(f"{str(p.relative_to(project_root)):<60} {'MISSING':>10}")
    print("=" * 70)
    print(
        "NOTE: MCP clients launch godot-mcp-server.exe directly from target/."
        "\n      Restart your MCP client (and the Godot editor) after a rebuild"
        "\n      to ensure the new binaries are picked up."
    )


def main():
    parser = argparse.ArgumentParser(
        description="Build and package the Godot MCP addon."
    )
    parser.add_argument(
        "--release", action="store_true", help="Build with --release profile"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Run `cargo clean` and wipe addons/godot_mcp/bin/ before building",
    )
    parser.add_argument(
        "--no-zip", action="store_true", help="Skip producing addons.zip"
    )
    parser.add_argument(
        "--no-server",
        action="store_true",
        help="Skip building godot-mcp-server (only rebuild the GDExtension)",
    )
    args = parser.parse_args()

    project_root = Path(__file__).parent.resolve()

    print(f"Project root: {project_root}")
    print(f"Profile: {'release' if args.release else 'debug'}")

    version = read_workspace_version(project_root)
    update_plugin_version(project_root, version)

    print("\n[KILL] terminating any running godot-mcp-server processes")
    kill_server_processes()

    if args.clean and not clean_artifacts(project_root):
        sys.exit(1)

    if not build_crates(project_root, args.release, args.no_server):
        print("[FATAL] build failed", file=sys.stderr)
        sys.exit(1)

    if not copy_library(project_root, args.release):
        sys.exit(1)

    packaged = False
    if not args.no_zip:
        if not package_addons(project_root, "addons.zip"):
            sys.exit(1)
        packaged = True
    else:
        print("\n[SKIP] addons.zip (--no-zip)")

    print_summary(project_root, args.release, packaged)


if __name__ == "__main__":
    main()
