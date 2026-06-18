"""CMake process wrapper — uses CMakePresets for configuration."""

import os
import subprocess
import sys
from collections.abc import Mapping
from pathlib import Path

from scripts._project import PROJECT_ROOT, BUILD_DIR

STALE_CACHE_PATTERNS = [
    "MSB4019",
    "VCTargetsPath",
    "CMAKE_C_COMPILER",
    "CMAKE_CXX_COMPILER",
]

SSL_ERROR_PATTERNS = [
    "CRYPT_E_REVOCATION_OFFLINE",
    "SSL connect error",
]

RETRYABLE_NETWORK_PATTERNS = [
    "Timed out",
    "Could not connect",
    "Connection was reset",
    "Recv failure",
    "Resolving timed out",
    "status_code: 28",
    "status_code: 56",
    "Failure when receiving data",
    "Failed to connect",
]


def run(cmd: list, capture: bool = False, extra_env: Mapping[str, str] | None = None) -> tuple[bool, str]:
    print(f"\n$ {' '.join(cmd)}", flush=True)
    env = {**os.environ, "PYTHONIOENCODING": "utf-8"}
    if extra_env:
        env.update(extra_env)
    if capture:
        result = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True, text=True,
                                encoding="utf-8", errors="replace", env=env)
        output = (result.stdout or "") + (result.stderr or "")
        if result.returncode != 0:
            print(output, flush=True)
        return result.returncode == 0, output
    else:
        return subprocess.run(cmd, cwd=PROJECT_ROOT, env=env).returncode == 0, ""


_last_binary_dir: str | None = None


def is_stale_cache_error(output: str) -> bool:
    return any(pat in output for pat in STALE_CACHE_PATTERNS)


def get_last_binary_dir() -> str:
    """Return binary dir from the most recent successful configure."""
    return _last_binary_dir or str(BUILD_DIR)


def configure(extra_defs: list, extra_env: Mapping[str, str] | None = None) -> tuple[bool, str]:
    """CMake configure using auto-detected CMakePreset.
    
    Detects environment → selects preset → runs cmake --preset.
    Falls back to manual -GNinja if preset fails.
    """
    global _last_binary_dir
    from scripts._detect import detect_environment
    from scripts._project import BUILD_DIR, PROJECT_ROOT
    
    env_info = detect_environment()
    preset_base = env_info["selected_preset"]
    vs_env = env_info["vs_env"]
    cache_name = env_info["cache_name"]
    generator = env_info["generator"]
    
    # Determine build type from extra_defs and append to preset name
    build_suffix = "debug"
    filtered_defs = []
    for d in extra_defs:
        if "CMAKE_BUILD_TYPE=Release" in d:
            build_suffix = "release"
        else:
            filtered_defs.append(d)
    preset_name = f"{preset_base}-{build_suffix}"
    
    # Determine the binary directory BEFORE running configure,
    # so build() can find it even if configure fails mid-way.
    if generator == "ninja":
        _last_binary_dir = str(BUILD_DIR / preset_name)
    else:
        _last_binary_dir = str(BUILD_DIR)
    
    final_env = {**(extra_env or {})}
    if vs_env:
        final_env.update(vs_env)
    
    # Build the cmake command
    cmd = ["cmake", "--preset", preset_name, "-S", str(PROJECT_ROOT)]
    
    # If generator is not ninja, we can't use presets that assume ninja.
    # Fall back to direct configuration.
    if generator != "ninja":
        cmd = ["cmake", "-B", _last_binary_dir, "-S", str(PROJECT_ROOT)] + filtered_defs
    else:
        cmd += filtered_defs
    
    # Override cache if auto-detected
    if cache_name:
        cmd.append(f"-DGODOTMCP_CACHE={cache_name}")
    
    print(f"[DETECT] OS: {env_info.get('compiler_id', '?')} | "
          f"Preset: {preset_name} | Cache: {cache_name or 'none'} | "
          f"Generator: {generator}", flush=True)
    
    ok, output = run(cmd, capture=True, extra_env=final_env if final_env else None)
    if ok:
        return True, ""
    
    # SSL error retry
    if any(p in output for p in SSL_ERROR_PATTERNS):
        print("\n[AUTO-SSL] SSL revocation check failed, retrying with CMAKE_TLS_VERIFY=0", flush=True)
        ssl_env = {**(final_env or {}), "CMAKE_TLS_VERIFY": "0"}
        ok, output = run(cmd, capture=True, extra_env=ssl_env if ssl_env else None)
        if ok:
            return True, ""
    
    # General network retry (timeout, connection reset, etc.)
    if any(p in output for p in RETRYABLE_NETWORK_PATTERNS):
        print("\n[AUTO-RETRY] Network error detected, retrying once...", flush=True)
        ok, output = run(cmd, capture=True, extra_env=final_env if final_env else None)
        if ok:
            return True, ""

    return False, output


def build(config: str, jobs: int, extra_env: Mapping[str, str] | None = None) -> bool:
    """CMake --build."""
    ok, _ = run(
        ["cmake", "--build", get_last_binary_dir(), "--config", config, "-j", str(jobs)],
        extra_env=extra_env,
    )
    return ok
