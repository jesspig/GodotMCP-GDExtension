import os
import tomllib
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

from godot_mcp_server.editor_ctl import (
    SERVER_VERSION,
    GODOT_PATH_MISSING_ERROR,
    kill_process_by_name,
    resolve_godot_path,
    resolve_project_path,
)

_server_dir = Path(__file__).parent.parent
with open(_server_dir / "pyproject.toml", "rb") as f:
    _expected_version = tomllib.load(f)["project"]["version"]


def test_server_version():
    assert SERVER_VERSION == _expected_version


def test_resolve_godot_path_with_override():
    assert resolve_godot_path("/usr/bin/godot") == "/usr/bin/godot"


def test_resolve_godot_path_missing():
    with patch.dict(os.environ, {}, clear=True):
        with pytest.raises(RuntimeError, match=GODOT_PATH_MISSING_ERROR[:20]):
            resolve_godot_path(None)


def test_resolve_godot_path_from_env():
    with patch.dict(os.environ, {"GODOT_PATH": "C:/godot.exe"}, clear=True):
        assert resolve_godot_path(None) == "C:/godot.exe"


def test_resolve_project_path_default():
    p = resolve_project_path({})
    assert p.endswith("example")


def test_resolve_project_path_absolute():
    p = resolve_project_path({"project_path": "C:/tmp/myproject"})
    assert os.path.isabs(p)


@patch("godot_mcp_server.editor_ctl.subprocess.run")
def test_kill_process_windows(mock_run):
    mock_run.return_value.returncode = 0
    import platform
    if platform.system() == "Windows":
        kill_process_by_name("godot.exe")
        assert mock_run.called
        args = mock_run.call_args[0][0]
        assert args[0] == "taskkill"
