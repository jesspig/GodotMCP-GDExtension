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


def find_apple_clang() -> list[dict]:
    try:
        result = subprocess.run(
            ["xcrun", "--find", "clang++"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode != 0 or not result.stdout.strip():
            return []
        compiler_path = result.stdout.strip()
        _ = subprocess.run(
            ["xcrun", "--show-sdk-path"],
            capture_output=True, text=True, timeout=5,
        )
        version = _get_version(compiler_path)
        if version:
            return [{
                "id": "appleclang",
                "type": "appleclang",
                "path": compiler_path,
                "version": version,
            }]
    except (FileNotFoundError, subprocess.TimeoutExpired, subprocess.CalledProcessError, OSError):
        pass
    return []


def find_homebrew_gcc() -> list[dict]:
    search_roots = [
        Path("/opt/homebrew/bin"),
        Path("/usr/local/bin"),
    ]
    found = []
    for root in search_roots:
        for ver in (14, 13, 12, 11):
            candidate = root / f"g++-{ver}"
            if not candidate.exists():
                continue
            version = _get_version(str(candidate))
            if version:
                found.append({
                    "id": f"gcc-{ver}",
                    "type": "gcc",
                    "path": str(candidate),
                    "version": version,
                })
    found.sort(key=lambda d: [int(x) for x in d["version"].split(".")], reverse=True)
    return found


def find_homebrew_clang() -> list[dict]:
    candidates = [
        Path("/opt/homebrew/opt/llvm/bin/clang++"),
        Path("/usr/local/opt/llvm/bin/clang++"),
    ]
    found = []
    for candidate in candidates:
        if not candidate.exists():
            continue
        version = _get_version(str(candidate))
        if version:
            found.append({
                "id": "clang-homebrew",
                "type": "clang",
                "path": str(candidate),
                "version": version,
            })
    return found


def find_apple_sdk_path() -> str | None:
    try:
        result = subprocess.run(
            ["xcrun", "--show-sdk-path"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    except (FileNotFoundError, subprocess.TimeoutExpired, subprocess.CalledProcessError, OSError):
        pass
    return None
