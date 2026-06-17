"""CMake 进程封装。

职责边界：
- 只关心"如何运行 cmake 命令"，不关心构建什么项目
- 依赖 _msvc 获取 vcvarsall 环境，仅在 Windows+Ninja 时生效
- configure() 包含 SSL 降级重试 + 过期缓存自动清理"""

import os
import platform
import shutil
import subprocess
import sys
from collections.abc import Mapping

from scripts._project import PROJECT_ROOT, BUILD_DIR
from scripts._msvc import find_msvc_cl, find_windows_sdk_tools, capture_vs_dev_env

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


def is_stale_cache_error(output: str) -> bool:
    return any(pat in output for pat in STALE_CACHE_PATTERNS)


def configure(extra_defs: list) -> tuple[bool, str]:
    """CMake configure，自动检测 Ninja/MSVC。

    返回 (success, output)。失败时调用方可自行决定是否 --clean 重试。
    """
    cmd = ["cmake", "-B", str(BUILD_DIR), "-S", str(PROJECT_ROOT)] + extra_defs
    extra_env = None

    if shutil.which("ninja"):
        if platform.system() == "Windows":
            cl_path = find_msvc_cl()
            if not cl_path:
                print("[DETECT] Ninja found but MSVC not found, using default generator", flush=True)
            else:
                extra_env = capture_vs_dev_env()
                cmd += ["-GNinja"]
                if not extra_env:
                    rc_path, mt_path = find_windows_sdk_tools()
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


def build(config: str, jobs: int, extra_env: Mapping[str, str] | None = None) -> bool:
    """CMake --build。"""
    ok, _ = run(
        ["cmake", "--build", str(BUILD_DIR), "--config", config, "-j", str(jobs)],
        extra_env=extra_env,
    )
    return ok
