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


def find_gcc_versions() -> list[dict]:
    candidates = [f"g++-{v}" for v in (14, 13, 12, 11, 10)] + ["g++"]
    found = []
    for name in candidates:
        path = shutil.which(name)
        if not path:
            continue
        version = _get_version(path)
        if version:
            found.append({
                "id": f"gcc-{version.split('.')[0]}" if version.split('.')[0] != Path(path).stem.removeprefix("g++-") else name,
                "type": "gcc",
                "path": path,
                "version": version,
            })
    for base in (Path("/usr/bin"), Path("/usr/local/bin")):
        for candidate in (base / "g++-14", base / "g++-13", base / "g++-12", base / "g++-11", base / "g++-10", base / "g++"):
            if not candidate.exists():
                continue
            if any(d["path"] == str(candidate) for d in found):
                continue
            version = _get_version(str(candidate))
            if version:
                found.append({
                    "id": f"gcc-{version.split('.')[0]}",
                    "type": "gcc",
                    "path": str(candidate),
                    "version": version,
                })
    found.sort(key=lambda d: [int(x) for x in d["version"].split(".")], reverse=True)
    return found


def find_clang_versions() -> list[dict]:
    candidates = [f"clang++-{v}" for v in (19, 18, 17, 16, 15)] + ["clang++"]
    found = []
    for name in candidates:
        path = shutil.which(name)
        if not path:
            continue
        version = _get_version(path)
        if version:
            found.append({
                "id": f"clang-{version.split('.')[0]}",
                "type": "clang",
                "path": path,
                "version": version,
            })
    for base in (Path("/usr/bin"), Path("/usr/local/bin")):
        for candidate in (base / "clang++-19", base / "clang++-18", base / "clang++-17", base / "clang++-16", base / "clang++-15", base / "clang++"):
            if not candidate.exists():
                continue
            if any(d["path"] == str(candidate) for d in found):
                continue
            version = _get_version(str(candidate))
            if version:
                found.append({
                    "id": f"clang-{version.split('.')[0]}",
                    "type": "clang",
                    "path": str(candidate),
                    "version": version,
                })
    found.sort(key=lambda d: [int(x) for x in d["version"].split(".")], reverse=True)
    return found


def find_lld() -> str | None:
    candidates = ["ld.lld-19", "ld.lld-18", "ld.lld-17", "ld.lld-16", "ld.lld-15",
                  "ld.lld-14", "ld.lld-13", "ld.lld-12", "lld-link", "ld.lld", "lld"]
    for name in candidates:
        path = shutil.which(name)
        if path:
            return path
    return None
