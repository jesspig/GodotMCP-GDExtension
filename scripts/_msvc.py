"""Windows MSVC 工具链检测。

Ninja + MSVC 需要先跑 vcvarsall.bat 注入 INCLUDE/LIB/PATH，
否则 cl.exe 找不到标准库头文件（fatal error C1034）。
Linux/macOS 不需要此模块，所有函数自然 no-op。"""

import json
import os
import platform
import shutil
import subprocess
from pathlib import Path

from scripts._project import PROJECT_ROOT, BUILD_DIR


_VS_DEV_ENV: dict[str, str] | None = None


def find_msvc_cl() -> str | None:
    """用 vswhere 或目录扫描查找 cl.exe 全路径。"""
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


def find_windows_sdk_tools() -> tuple[str | None, str | None]:
    """找到 Windows SDK 的 rc.exe 和 mt.exe。"""
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
    """找到 VS MSBuild VC 目标目录，修复 .NET SDK 干扰。"""
    try:
        result = subprocess.run(
            ["vswhere", "-latest", "-property", "installationPath"],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        if result.returncode == 0 and result.stdout.strip():
            vs_path = Path(result.stdout.strip())
            for ver in ("v180", "v170", "v160"):
                vc_path = vs_path / "MSBuild" / "Microsoft" / "VC" / ver
                if vc_path.exists():
                    return str(vc_path) + "\\"
    except FileNotFoundError:
        pass
    return None


def capture_vs_dev_env() -> dict[str, str] | None:
    """执行 vcvarsall.bat x64 并捕获环境变量。"""
    global _VS_DEV_ENV
    if _VS_DEV_ENV is not None:
        return _VS_DEV_ENV
    cl_path = find_msvc_cl()
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
    bat_path = BUILD_DIR / f"_vs_env_{os.getpid()}.bat"
    bat_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        bat_path.write_text(bat_content, encoding="utf-8")
        result = subprocess.run([str(bat_path)], capture_output=True, text=True, encoding="utf-8", errors="replace")
        if result.returncode == 0 and result.stdout:
            _VS_DEV_ENV = json.loads(result.stdout)
    finally:
        bat_path.unlink(missing_ok=True)
    return _VS_DEV_ENV


def get_vs_env() -> dict[str, str] | None:
    """若 Windows 上可用 Ninja + MSVC，返回 vcvarsall 环境；否则返回 None。"""
    if platform.system() == "Windows" and shutil.which("ninja") and find_msvc_cl():
        return capture_vs_dev_env()
    return None


def ensure_vctargetspath() -> None:
    """若 VCTargetsPath 未设置，自动检测并注入环境变量。"""
    if "VCTargetsPath" not in os.environ:
        vc_path = detect_vctargetspath()
        if vc_path:
            print(f"\n[ENV] VCTargetsPath={vc_path}", flush=True)
            os.environ["VCTargetsPath"] = vc_path
