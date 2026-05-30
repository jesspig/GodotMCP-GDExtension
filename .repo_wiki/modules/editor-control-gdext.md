# Editor Control 命令（gdext 侧）

> 6 个编辑器控制工具，在 gdext 进程内执行。实现在 `extensions/gdext/src/commands/editor_control.cpp`。

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
