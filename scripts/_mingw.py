import re
import shutil
import subprocess
from pathlib import Path


def _get_version(compiler_path: str) -> str | None:
    try:
        result = subprocess.run(
            [compiler_path, "--version"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0:
            m = re.search(r"(\d+\.\d+\.\d+)", result.stdout)
            if m:
                return m.group(1)
            m = re.search(r"(\d+\.\d+)", result.stdout)
            if m:
                return m.group(1)
    except (FileNotFoundError, subprocess.TimeoutExpired, subprocess.CalledProcessError, OSError):
        pass
    return None


def find_mingw_arch(compiler_path: str) -> str:
    name = Path(compiler_path).name
    if "x86_64" in name:
        return "x86_64"
    if "i686" in name:
        return "i686"
    try:
        result = subprocess.run(
            [compiler_path, "-dumpmachine"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0:
            output = result.stdout.strip()
            if "x86_64" in output:
                return "x86_64"
            if "i686" in output or "i386" in output:
                return "i686"
    except (FileNotFoundError, subprocess.TimeoutExpired, subprocess.CalledProcessError, OSError):
        pass
    return "x86_64"


def find_mingw_gcc() -> list[dict]:
    candidates = [
        "x86_64-w64-mingw32-g++",
        "x86_64-w64-mingw32-c++",
        "i686-w64-mingw32-g++",
        "i686-w64-mingw32-c++",
    ]
    found = []
    for name in candidates:
        path = shutil.which(name)
        if path:
            version = _get_version(path)
            if version:
                arch = find_mingw_arch(path)
                found.append({
                    "id": f"mingw{arch}",
                    "type": "gcc",
                    "path": path,
                    "version": version,
                })

    search_roots = [
        Path("C:/msys64/mingw64/bin"),
        Path("C:/msys64/mingw32/bin"),
        Path("C:/tools/msys64/mingw64/bin"),
        Path("C:/tools/msys64/mingw32/bin"),
        Path("C:/cygwin64/bin"),
        Path("C:/tools/cygwin/bin"),
    ]
    for root in search_roots:
        if not root.exists():
            continue
        for name in ("x86_64-w64-mingw32-g++.exe", "x86_64-w64-mingw32-c++.exe",
                     "i686-w64-mingw32-g++.exe", "i686-w64-mingw32-c++.exe"):
            candidate = root / name
            if not candidate.exists():
                continue
            if any(d["path"] == str(candidate) for d in found):
                continue
            version = _get_version(str(candidate))
            if version:
                arch = find_mingw_arch(str(candidate))
                found.append({
                    "id": f"mingw{arch}",
                    "type": "gcc",
                    "path": str(candidate),
                    "version": version,
                })

    def sort_key(d):
        arch_order = 0 if "x86_64" in d["id"] else 1
        ver_parts = [int(x) for x in d["version"].split(".")]
        return (arch_order, [-v for v in ver_parts])

    found.sort(key=sort_key)
    return found
