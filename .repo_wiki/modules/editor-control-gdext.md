# Editor Control 命令（gdext 侧）

> 6 个编辑器控制工具，在 gdext 进程内执行（区别于 server 侧的 `godot_editor_open/close/restart`）。实现在 `extensions/gdext/src/commands/editor_control.cpp`。

## 工具列表

| 工具 | 描述 | 返回 |
|------|------|------|
| `play_current_scene` | 播放当前编辑器中打开的场景 | `{"playing": true}` |
| `play_main_scene` | 播放项目主场景 | `{"playing": true, "main_scene": "..."}` |
| `stop_scene` | 停止正在运行的场景 | `{"stopped": true}` |
| `is_scene_playing` | 查询场景是否正在播放 | `{"playing": bool, "scene_path": Value}` |
| `refresh_filesystem` | 刷新编辑器文件系统扫描 | `{"scanning": true}` |
| `get_editor_info` | 获取编辑器信息 | 见下方 |

## `get_editor_info` 返回值

```json
{
  "engine_version": "4.6.0",
  "editor_scale": 1.0,
  "editor_language": "en",
  "editor_paths": {
    "data_dir": "C:/Users/...",
    "config_dir": "C:/Users/...",
    "cache_dir": "C:/Users/...",
    "project_settings_dir": "C:/Users/..."
  }
}
```

## 与服务器端 EditorControl 的区别

| | 服务器端 (handler.py) | gdext 侧 (editor_control.cpp) |
|---|---|---|
| 进程 | godot-mcp-server | godot_mcp_gdext.dll |
| 工具 | `godot_editor_open`/`close`/`restart` | `play_*`/`stop_scene`/`refresh_filesystem`/`get_editor_info` |
| 功能 | 启动/关闭/重启编辑器进程 | 控制编辑器内的场景播放、文件系统、查询 |
