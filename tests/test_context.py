from dataclasses import dataclass, field
from typing import Any


@dataclass
class TestContext:
    session: Any = None
    available_tools: set[str] = field(default_factory=set)
    dotnet_version: str = ""
    dotnet_available: bool = False
    godot_process: Any = None
    project_path: str = ""

    created_files: list[str] = field(default_factory=list)
    created_nodes: list[str] = field(default_factory=list)
    test_input_actions: list[str] = field(default_factory=list)
    test_autoloads: list[str] = field(default_factory=list)

    original_project_godot: str = ""
    original_settings: dict[str, Any] = field(default_factory=dict)

    temp_scene_path: str = ""
    temp_scene_root_name: str = ""

    is_playing: bool = False
