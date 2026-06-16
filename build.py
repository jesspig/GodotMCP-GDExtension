#!/usr/bin/env python3
"""Thin wrapper around CMake + CPack for building and packaging GodotMCP.

Usage:
    uv run python build.py                               # debug build + addons.zip
    uv run python build.py --release                     # release build + addons.zip
    uv run python build.py --no-zip                      # build only, skip zip
    uv run python build.py --clean                       # wipe build/ (keeps _deps/)
    uv run python build.py --clean-all                   # remove entire build/ (incl. _deps/)
    uv run python build.py --purge-cache                 # also wipe _deps/ (force re-download)
"""

import argparse
import os
import shutil
import subprocess
import sys
import multiprocessing
import platform
from pathlib import Path
import json

PROJECT_ROOT = Path(__file__).parent.resolve()
BUILD_DIR = PROJECT_ROOT / "build"

# Error patterns that indicate a stale CMake cache and need a clean reconfigure
STALE_CACHE_PATTERNS = [
    "MSB4019",          # .NET SDK path mismatch (Microsoft.Cpp.Default.props)
    "VCTargetsPath",    # VC++ targets path resolution failure
    "CMAKE_C_COMPILER", # Compiler path changed
    "CMAKE_CXX_COMPILER",
]

SSL_ERROR_PATTERNS = [
    "CRYPT_E_REVOCATION_OFFLINE",  # schannel CRL check failed (codeload.github.com)
    "SSL connect error",           # generic SSL handshake failure
]


def _find_msvc_cl() -> str | None:
    """Find MSVC cl.exe using vswhere, falling back to directory scanning."""
    try:
        result = subprocess.run(
            ["vswhere", "-latest", "-property", "installationPath"],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        if result.returncode == 0 and result.stdout.strip():
            vs_path = Path(result.stdout.strip())
            msvc_root = vs_path / "VC" / "Tools" / "MSVC"
            if msvc_root.exists():
                for ver in sorted(msvc_root.iterdir(), reverse=True):
                    cl_exe = ver / "bin" / "Hostx64" / "x64" / "cl.exe"
                    if cl_exe.exists():
                        return str(cl_exe)
    except FileNotFoundError:
        pass
    candidate_roots = [
        Path("C:/Program Files/Microsoft Visual Studio"),
        Path("C:/Program Files (x86)/Microsoft Visual Studio"),
    ]
    for root in candidate_roots:
        if not root.exists():
            continue
        for d1 in root.iterdir():
            items = [d1] + sorted(d1.iterdir()) if d1.is_dir() else [d1]
            for d2 in items:
                msvc_dir = Path(d2) / "VC" / "Tools" / "MSVC"
                if not msvc_dir.exists():
                    continue
                for ver in sorted(msvc_dir.iterdir(), reverse=True):
                    cl_exe = ver / "bin" / "Hostx64" / "x64" / "cl.exe"
                    if cl_exe.exists():
                        return str(cl_exe)
    return None


def _find_windows_sdk_tools() -> tuple[str | None, str | None]:
    """Find rc.exe and mt.exe from the latest Windows SDK."""
    sdk_roots = [
        Path("C:/Program Files (x86)/Windows Kits/10/bin"),
        Path("C:/Program Files/Windows Kits/10/bin"),
    ]
    for root in sdk_roots:
        if not root.exists():
            continue
        versions = sorted(root.iterdir(), reverse=True)
        for ver in versions:
            x64 = ver / "x64"
            if not x64.exists():
                continue
            rc = x64 / "rc.exe"
            mt = x64 / "mt.exe"
            rc_str = str(rc) if rc.exists() else None
            mt_str = str(mt) if mt.exists() else None
            if rc_str or mt_str:
                return rc_str, mt_str
    return None, None


def detect_vctargetspath() -> str | None:
    """Detect VCTargetsPath by finding Visual Studio's MSBuild VC directory."""
    try:
        result = subprocess.run(
            ["vswhere", "-latest", "-property", "installationPath"],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        if result.returncode == 0 and result.stdout.strip():
            vs_path = Path(result.stdout.strip())
            # Try v180 (VS 2026), v170 (VS 2022), v160 (VS 2019) — in order of preference.
            for ver in ("v180", "v170", "v160"):
                vc_path = vs_path / "MSBuild" / "Microsoft" / "VC" / ver
                if vc_path.exists():
                    return str(vc_path) + "\\"
    except FileNotFoundError:
        pass
    return None


# Cache for VS dev environment (vcvarsall)
_VS_DEV_ENV: dict[str, str] | None = None


def _capture_vs_dev_env() -> dict[str, str] | None:
    """Capture the VS developer command prompt environment via vcvarsall.bat."""
    global _VS_DEV_ENV
    if _VS_DEV_ENV is not None:
        return _VS_DEV_ENV
    cl_path = _find_msvc_cl()
    if not cl_path:
        return None
    cl = Path(cl_path)
    vcvarsall = None
    for parent in cl.parents:
        candidate = parent / "Auxiliary" / "Build" / "vcvarsall.bat"
        if candidate.exists():
            vcvarsall = candidate
            break
    if not vcvarsall:
        return None
    bat_content = f'@call "{vcvarsall}" x64 >nul 2>&1\n@python -c "import json,os; print(json.dumps(dict(os.environ)))"'
    bat_path = PROJECT_ROOT / "build" / f"_vs_env_{os.getpid()}.bat"
    bat_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        bat_path.write_text(bat_content, encoding="utf-8")
        result = subprocess.run([str(bat_path)], capture_output=True, text=True, encoding="utf-8", errors="replace")
        if result.returncode == 0 and result.stdout:
            _VS_DEV_ENV = json.loads(result.stdout)
    finally:
        bat_path.unlink(missing_ok=True)
    return _VS_DEV_ENV


def run(cmd: list, capture: bool = False, extra_env: dict[str, str] | None = None) -> tuple[bool, str]:
    print(f"\n$ {' '.join(cmd)}", flush=True)
    env = {**os.environ, "PYTHONIOENCODING": "utf-8"}
    if extra_env:
        env.update(extra_env)
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



def _get_vs_env() -> dict[str, str] | None:
    """Return VS dev environment if using Ninja on Windows, else None."""
    if platform.system() == "Windows" and shutil.which("ninja") and _find_msvc_cl():
        return _capture_vs_dev_env()
    return None


def configure(extra_defs: list) -> tuple[bool, str]:
    """Run CMake configure. Returns (success, output).

    Auto-retries with CMAKE_TLS_VERIFY=0 when schannel SSL revocation
    checks fail (common on networks blocking CRL endpoints).
    """
    cmd = ["cmake", "-B", str(BUILD_DIR), "-S", str(PROJECT_ROOT)] + extra_defs
    extra_env = None
    # Auto-detect Ninja generator for faster incremental builds.
    # CMake auto-detects MSVC from the vcvarsall environment (no explicit
    # -DCMAKE_CXX_COMPILER needed) — this avoids hardcoding a specific
    # MSVC toolchain version.
    if shutil.which("ninja"):
        if platform.system() == "Windows":
            cl_path = _find_msvc_cl()
            if not cl_path:
                print("[DETECT] Ninja found but MSVC not found, using default generator", flush=True)
            else:
                extra_env = _capture_vs_dev_env()
                cmd += ["-GNinja"]
                if not extra_env:
                    rc_path, mt_path = _find_windows_sdk_tools()
                    if rc_path:
                        cmd += ["-DCMAKE_RC_COMPILER:FILEPATH=" + rc_path]
                    if mt_path:
                        cmd += ["-DCMAKE_MT:FILEPATH=" + mt_path]
                print("[DETECT] Ninja + MSVC found, using -GNinja", flush=True)
        else:
            cmd += ["-GNinja"]
            print("[DETECT] Ninja found, using -GNinja", flush=True)
    else:
        print("[DETECT] Ninja not found, using default generator", flush=True)
    ok, output = run(cmd, capture=True, extra_env=extra_env)
    if ok:
        return True, ""
    if any(p in output for p in SSL_ERROR_PATTERNS):
        print("\n[AUTO-SSL] SSL revocation check failed, retrying with CMAKE_TLS_VERIFY=0", flush=True)
        ssl_env = {**(extra_env or {}), "CMAKE_TLS_VERIFY": "0"}
        ok, output = run(cmd, capture=True, extra_env=ssl_env)
        if ok:
            return True, ""
    return False, output


def main():
    parser = argparse.ArgumentParser(description="Build and package GodotMCP via CMake")
    parser.add_argument("--release", action="store_true", help="Build with Release configuration")
    parser.add_argument("--clean", action="store_true", help="Wipe build/ cache (preserves _deps/ FetchContent cache)")
    parser.add_argument("--clean-all", action="store_true", help="Remove entire build/ directory (including _deps/ — re-downloads deps)")
    parser.add_argument("--purge-cache", action="store_true", help="Remove build/_deps/ only (re-downloads deps, keeps build cache)")
    parser.add_argument("--no-zip", action="store_true", help="Skip producing addons.zip")
    parser.add_argument("-j", "--jobs", type=int, default=None, help="Number of parallel build jobs (default: auto-detect CPU count)")
    args = parser.parse_args()

    config = "Release" if args.release else "Debug"
    cmake_defs = []
    if args.release:
        cmake_defs += ["-DCMAKE_BUILD_TYPE=Release"]

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
        for item in BUILD_DIR.iterdir():
            if item.name == "_deps":
                continue
            if item.is_dir():
                shutil.rmtree(item, ignore_errors=True)
            else:
                item.unlink(missing_ok=True)

    # --- Ensure VCTargetsPath is set (fixes .NET SDK preview interference) ---
    if "VCTargetsPath" not in os.environ:
        vc_path = detect_vctargetspath()
        if vc_path:
            print(f"\n[ENV] VCTargetsPath={vc_path}", flush=True)
            os.environ["VCTargetsPath"] = vc_path

    # --- Configure ---
    ok, output = configure(cmake_defs)
    if not ok:
        # Auto-retry with clean if stale cache detected
        if BUILD_DIR.exists() and is_stale_cache_error(output):
            print("\n[AUTO-CLEAN] Stale build cache detected, reconfiguring...", flush=True)
            shutil.rmtree(BUILD_DIR, ignore_errors=True)
            ok, output = configure(cmake_defs)
        if not ok:
            print("\n[HINT] Try: uv run python build.py --clean", flush=True)
            sys.exit(1)

    # --- Build ---
    n_cpus = multiprocessing.cpu_count()
    if args.jobs is not None:
        if args.jobs > n_cpus:
            print(f"[BUILD] -j {args.jobs} exceeds {n_cpus} CPUs, using {n_cpus}", flush=True)
        elif args.jobs < 1:
            print("[BUILD] Error: -j must be >= 1", flush=True)
            sys.exit(1)
        else:
            n_cpus = args.jobs
            print(f"[BUILD] Using -j {args.jobs}", flush=True)
    if not run(["cmake", "--build", str(BUILD_DIR), "--config", config, "-j", str(n_cpus)], extra_env=_get_vs_env()):
        sys.exit(1)

    # --- Package ---
    if not args.no_zip:
        if not run(["cmake", "--build", str(BUILD_DIR), "--config", config, "--target", "package", "-j", str(n_cpus)]):
            sys.exit(1)
    else:
        print("\n[SKIP] addons.zip (--no-zip)")


if __name__ == "__main__":
    main()

