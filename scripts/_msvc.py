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


def _find_vs_path() -> Path | None:
    """用 vswhere 找到最新 VS 安装根目录。"""
    try:
        result = subprocess.run(
            ["vswhere", "-latest", "-property", "installationPath"],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        if result.returncode == 0 and result.stdout.strip():
            return Path(result.stdout.strip())
    except FileNotFoundError:
        pass
    return None


def _scan_vs_roots() -> list[Path]:
    """扫描已知 VS 安装目录（vswhere 不可用时兜底）。"""
    roots = []
    for base in [
        Path("C:/Program Files/Microsoft Visual Studio"),
        Path("C:/Program Files (x86)/Microsoft Visual Studio"),
    ]:
        if not base.exists():
            continue
        for d1 in base.iterdir():
            items = [d1] + sorted(d1.iterdir()) if d1.is_dir() else [d1]
            roots.extend(items)
    return roots


def find_clang_cl() -> str | None:
    """优先检测 clang-cl（VS Llvm 工具集 > 独立 LLVM 安装 > PATH）。"""
    vs_path = _find_vs_path()
    if vs_path:
        clang_cl_exe = vs_path / "VC" / "Tools" / "Llvm" / "bin" / "clang-cl.exe"
        if clang_cl_exe.exists():
            return str(clang_cl_exe)
    for root in _scan_vs_roots():
        clang_cl_exe = Path(root) / "VC" / "Tools" / "Llvm" / "bin" / "clang-cl.exe"
        if clang_cl_exe.exists():
            return str(clang_cl_exe)
    for llvm_root in [
        Path("C:/Program Files/LLVM"),
        Path("C:/Program Files (x86)/LLVM"),
    ]:
        clang_cl_exe = llvm_root / "bin" / "clang-cl.exe"
        if clang_cl_exe.exists():
            return str(clang_cl_exe)
    clang_cl = shutil.which("clang-cl")
    if clang_cl:
        return clang_cl
    return None


def find_msvc_cl() -> str | None:
    """用 vswhere 或目录扫描查找 cl.exe 全路径。"""
    vs_path = _find_vs_path()
    if vs_path:
        msvc_root = vs_path / "VC" / "Tools" / "MSVC"
        if msvc_root.exists():
            for ver in sorted(msvc_root.iterdir(), reverse=True):
                cl_exe = ver / "bin" / "Hostx64" / "x64" / "cl.exe"
                if cl_exe.exists():
                    return str(cl_exe)
    for root in _scan_vs_roots():
        msvc_dir = Path(root) / "VC" / "Tools" / "MSVC"
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


def _find_vcvarsall() -> Path | None:
    """找 vcvarsall.bat（不依赖具体编译器路径）。"""
    vs_path = _find_vs_path()
    if vs_path:
        candidate = vs_path / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
        if candidate.exists():
            return candidate
    for root in _scan_vs_roots():
        candidate = Path(root) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
        if candidate.exists():
            return candidate
    return None


def capture_vs_dev_env() -> dict[str, str] | None:
    """执行 vcvarsall.bat x64 并捕获环境变量。"""
    global _VS_DEV_ENV
    if _VS_DEV_ENV is not None:
        return _VS_DEV_ENV
    vcvarsall = _find_vcvarsall()
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


# --- New functions for unified compiler scanning ---


def get_vs_year_from_version(ver: str) -> int:
    mapping = {
        "14.4": 2025, "14.3": 2022, "14.2": 2019,
        "14.1": 2017, "14.0": 2015,
    }
    for prefix, year in mapping.items():
        if ver.startswith(prefix):
            return year
    return 0


def find_all_msvc_versions() -> list[dict]:
    """Find all installed MSVC versions. Returns list sorted by version descending."""
    results = []
    vs_path = _find_vs_path()
    roots = [vs_path] if vs_path else []

    for root in _scan_vs_roots():
        if root not in roots:
            roots.append(root)

    for root in roots:
        msvc_dir = Path(root) / "VC" / "Tools" / "MSVC"
        if not msvc_dir.exists():
            continue
        for ver_dir in sorted(msvc_dir.iterdir(), reverse=True):
            for host_arch in ["Hostx64", "Hostx86"]:
                for target_arch in ["x64", "x86", "arm64", "arm"]:
                    cl_path = ver_dir / "bin" / host_arch / target_arch / "cl.exe"
                    if cl_path.exists():
                        results.append({
                            "id": f"msvc-{ver_dir.name}",
                            "type": "msvc",
                            "path": str(cl_path),
                            "version": ver_dir.name,
                            "priority": 2 if ver_dir.name.startswith(("14.4", "14.3")) else 3,
                        })
                        break
                if cl_path.exists():
                    break
    return results


def find_available_compilers() -> list[dict]:
    """Return all available Windows compilers (clang-cl + MSVC)."""
    compilers = []

    clang_cl_path = find_clang_cl()
    if clang_cl_path:
        compilers.append({
            "id": "clang-cl",
            "type": "clang",
            "path": clang_cl_path,
            "version": "clang (clang-cl)",
            "priority": 1,
        })

    for msvc in find_all_msvc_versions():
        compilers.append(msvc)

    return sorted(compilers, key=lambda c: c["priority"])
