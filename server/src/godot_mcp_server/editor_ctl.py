import os
import platform
import subprocess
import tomllib
from pathlib import Path

GODOT_PATH_MISSING_ERROR = (
    "GODOT_PATH 环境变量未设置。请在 MCP 客户端配置中添加:\n"
    '"env": { "GODOT_PATH": "<path/to/godot.exe>" }'
)

# pyproject.toml is at the project root (one level above the server/ dir).
_project_root = Path(__file__).parent.parent.parent.parent
with open(_project_root / "pyproject.toml", "rb") as f:
    _pyproject = tomllib.load(f)
SERVER_VERSION = _pyproject["project"]["version"]


def resolve_godot_path(override_path: str | None = None) -> str:
    if override_path:
        return override_path
    path = os.environ.get("GODOT_PATH")
    if not path:
        raise RuntimeError(GODOT_PATH_MISSING_ERROR)
    return path


def resolve_project_path(args: dict) -> str:
    p = args.get("project_path", "example")
    path = Path(p)
    if path.is_absolute():
        return str(path)
    return str(Path.cwd() / p)


def kill_process_by_name(name: str) -> bool:
    if platform.system() == "Windows":
        result = subprocess.run(
            ["taskkill", "/F", "/IM", name],
            capture_output=True,
            text=True,
        )
        return result.returncode == 0
    else:
        result = subprocess.run(
            ["pkill", "-x", name],
            capture_output=True,
            text=True,
        )
        return result.returncode == 0


def godot_editor_open(
    godot_path: str,
    project_path: str,
) -> dict:
    if not Path(project_path).exists():
        raise RuntimeError(f"项目目录不存在: {project_path}")
    try:
        child = subprocess.Popen(
            [godot_path, "--editor", "--path", project_path],
        )
    except FileNotFoundError:
        raise RuntimeError(
            f"启动 Godot 编辑器失败。请检查 GODOT_PATH 环境变量指向的文件是否存在"
            f"（当前值: {godot_path}），"
            f"如果 Godot 已升级导致文件名变更，请更新 GODOT_PATH。"
        )
    return {
        "status": "opened",
        "pid": child.pid,
        "godot_path": godot_path,
        "project_path": project_path,
    }


def godot_editor_close(godot_path: str) -> dict:
    exe_name = Path(godot_path).name
    killed = kill_process_by_name(exe_name)
    return {
        "status": "closed" if killed else "not_running",
        "process_name": exe_name,
    }
