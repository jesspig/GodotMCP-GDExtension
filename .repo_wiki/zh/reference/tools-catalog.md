# 工具目录

> 全部 99 个 MCP 工具的描述、参数和返回值。分为 13 个处理器分组 + 3 个服务器端工具。

## 分组速览

| 组 | 计数 | 路由 |
|----|------|------|
| [Editor Control](#editor-control-服务器端-3) | 3 | server-side |
| [MetaCommands](#metacommands-4) | 4 | gdext |
| [NodeCommands](#nodecommands-17) | 17 | gdext |
| [PropertyCommands](#propertycommands-19) | 19 | gdext |
| [CollisionCommands](#collisioncommands-2) | 2 | gdext |
| [FindCommands](#findcommands-4) | 4 | gdext |
| [ScriptHelpersCommands](#scripthelperscommands-3) | 3 | gdext |
| [ProjectSettingsCommands](#projectsettingscommands-3) | 3 | gdext |
| [SceneCommands](#scenecommands-15) | 15 | gdext |
| [ScriptGdCommands](#scriptgdcommands-5) | 5 | gdext |
| [ScriptCsCommands](#scriptcscommands-6) | 6 | gdext |
| [SearchCommands](#searchcommands-3) | 3 | gdext |
| [UndoCommands](#undocommands-2) | 2 | gdext |
| [Property3dCommands](#property3dcommands-6) | 6 | gdext |
| [NodeConvenience](#nodeconvenience-4) | 4 | gdext |
| [SceneInfo](#sceneinfo-1) | 1 | gdext |
| **总计** | **99** | |

---

## Editor Control (服务器端, 3)

需要 `GODOT_PATH` 环境变量。

### `godot_editor_open`
启动 Godot 编辑器并打开项目。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `project_path` | string | 否 | `godot/` |

**返回**: `{"status": "ok"}`

### `godot_editor_close`
关闭 Godot 编辑器进程。

**返回**: `{"status": "ok"}`

### `godot_editor_restart`
重启 Godot 编辑器。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `project_path` | string | 否 | `godot/` |

**返回**: `{"status": "ok"}`

---

## MetaCommands (4)

### `ping`
检测与 Godot 编辑器的连接状态。

**返回**: `{"status": "ok", "message": "pong"}`

### `get_engine_version`
获取 Godot 引擎版本号。

**返回**: `{"major": 4, "minor": 6, "patch": 0, "string": "4.6"}`

### `get_plugin_version`
获取 Godot MCP 插件版本号。

**返回**: `{"version": "x.y.z"}`

### `get_server_version`
获取 godot-mcp-server 版本号。

**返回**: `{"version": "x.y.z"}`

---

## NodeCommands (17)

### `create_node`
创建新节点。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `parent_path` | string | 是 | |
| `node_type` | string | 是 | |
| `name` | string | 是 | |

**返回**: `{"status": "ok", "name": "..."}`

### `delete_node`
删除节点。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"status": "ok"}`

### `duplicate_node`
复制节点。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"status": "ok", "duplicated_name": "..."}`

### `rename_node`
重命名节点。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `new_name` | string | 是 |

**返回**: `{"status": "ok"}`

### `move_node`
移动节点到新父节点。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `new_parent_path` | string | 是 |

**返回**: `{"status": "ok"}`

### `reset_parent`
重设父节点 (reparent)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `new_parent_path` | string | 是 |

**返回**: `{"status": "ok"}`

### `set_as_root`
设置节点为场景根。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"status": "ok"}` — 验证根节点已更改后返回。

### `attach_script`
为节点挂载脚本。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `script_path` | string | 是 |

**返回**: `{"status": "ok"}`

### `detach_script`
卸载节点脚本。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"status": "ok"}`

### `add_node_to_group`
添加节点到组。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `group` | string | 是 |

**返回**: `{"status": "ok"}`

### `remove_node_from_group`
从组中移除节点。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `group` | string | 是 |

**返回**: `{"status": "ok"}`

### `set_node_unique_name`
设置节点唯一名称（%前缀）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `unique` | boolean | 是 |

**返回**: `{"status": "ok"}`

### `get_scene_tree`
获取当前场景节点树。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `max_depth` | integer | 否 | 无限制 |

**返回**: `{"nodes": [{"name":"...", "type":"...", "children":[...]}]}`

### `get_node_path`
获取节点路径。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"path": "..."}`

### `get_node_info`
返回节点完整信息。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"type":"...", "script":"...", "visible":true, "groups":[], "child_count":3, "owner":"...", "unique_name":false, "is_instance":false}`

### `branch_to_scene`
将节点分支转为场景文件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `scene_path` | string | 是 |

**返回**: `{"status": "ok", "scene_path": "..."}`

### `scene_to_branch`
将实例化场景转为本地分支。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"status": "ok"}`

---

## PropertyCommands (19)

### `get_property`
获取节点指定属性。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `property` | string | 是 |

**返回**: `{"value": ...}`

### `set_property`
修改节点属性（通用兜底）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `property` | string | 是 |
| `value` | any | 是 |

**返回**: `{"status": "ok"}`

### `get_property_list`
获取节点所有属性列表。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"properties": [{"name":"...", "type":..., "hint":...}]}`

### `batch_set_property`
批量修改节点属性。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_paths` | array[string] | 是 |
| `property` | string | 是 |
| `value` | any | 是 |

**返回**: `{"status": "ok"}`

### `get_node_position`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"x": 100, "y": 200}`

### `set_node_position`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 是 |
| `y` | number | 是 |

**返回**: `{"status": "ok"}`

### `get_node_rotation`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"degrees": 45.0}`

### `set_node_rotation`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `degrees` | number | 是 |

**返回**: `{"status": "ok"}`

### `get_node_scale`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"x": 1.0, "y": 1.0}`

### `set_node_scale`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 是 |
| `y` | number | 是 |

**返回**: `{"status": "ok"}`

### `get_node_modulate`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"r": 1.0, "g": 1.0, "b": 1.0, "a": 1.0}`

### `set_node_modulate`
| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `r` | number | 否 | 1.0 |
| `g` | number | 否 | 1.0 |
| `b` | number | 否 | 1.0 |
| `a` | number | 否 | 1.0 |

**返回**: `{"status": "ok"}`

### `get_node_visible`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"visible": true}`

### `set_node_visible`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `visible` | boolean | 是 |

**返回**: `{"status": "ok"}`

### `get_node_text`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"text": "Hello"}`

### `set_node_text`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `text` | string | 是 |

**返回**: `{"status": "ok"}`

### `get_node_z_index`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"z_index": 0}`

### `set_node_z_index`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `z_index` | integer | 是 |

**返回**: `{"status": "ok"}`

### `call_method`
调用节点方法。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `method` | string | 是 |
| `args` | array | 否 | `[]` |

**返回**: `{"result": ...}`

---

## CollisionCommands (2)

### `add_circle_collision`
为节点添加圆形碰撞体。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `radius` | number | 否 | 自动计算 |

**返回**: `{"status": "ok", "mode": "create|detect_existing"}`

### `add_rectangle_collision`
为节点添加矩形碰撞体。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `width` | number | 否 | 自动检测 |
| `height` | number | 否 | 自动检测 |

**返回**: `{"status": "ok", "mode": "create|detect_existing"}`

---

## FindCommands (4)

### `find_nodes_by_name`
按名称子串搜索节点。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `pattern` | string | 是 |
| `max_results` | integer | 否 | 无限制 |

**返回**: `{"matches": [{"name":"...","path":"..."}]}`

### `find_nodes_by_type`
按节点类型精确搜索。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_class` | string | 是 |
| `max_results` | integer | 否 | 无限制 |

**返回**: `{"matches": [{"name":"...","path":"..."}]}`

### `find_nodes_by_group`
按组名搜索节点。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `group` | string | 是 |
| `max_results` | integer | 否 | 无限制 |

**返回**: `{"matches": [{"name":"...","path":"..."}]}`

### `find_nodes_by_script`
按脚本路径搜索节点。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `script_path` | string | 是 |
| `max_results` | integer | 否 | 无限制 |

**返回**: `{"matches": [{"name":"...","path":"..."}]}`

---

## ScriptHelpersCommands (3)

### `get_script_variables`
列出节点脚本上所有 @export 变量。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"variables": [{"name":"speed","type":"int","value":100}]}`

### `get_variable`
读取节点脚本变量（编辑器模式下仅支持 @export 变量）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `variable` | string | 是 |

**返回**: `{"value": ...}`

### `set_variable`
写入节点脚本变量（编辑器模式下仅支持 @export 变量）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `variable` | string | 是 |
| `value` | any | 是 |

**返回**: `{"status": "ok"}`

---

## ProjectSettingsCommands (3)

### `get_project_setting`
读取项目设置。

| 参数 | 类型 | 必须 |
|------|------|------|
| `property` | string | 是 |

**返回**: `{"value": ...}`

### `set_project_setting`
写入项目设置。自动调用 `ProjectSettings::save()`。

| 参数 | 类型 | 必须 |
|------|------|------|
| `property` | string | 是 |
| `value` | any | 是 |

**返回**: `{"status": "ok"}` — 如果保存失败，附加 `"warning": "保存失败: ..."` 字段。

### `set_main_scene`
设置主场景。

| 参数 | 类型 | 必须 |
|------|------|------|
| `scene_path` | string | 是 |

**返回**: `{"status": "ok"}`

---

## SceneCommands (15)

### `create_scene`
创建新场景文件。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `path` | string | 是 |
| `root_type` | string | 否 | `Node` |
| `root_name` | string | 否 | 文件名 |

**返回**: `{"status": "ok", "scene_path": "..."}`

### `open_scene`
在编辑器中打开场景文件。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `scene_path` | string | 是 |
| `set_inherited` | boolean | 否 | `false` |

**返回**: `{"status": "ok"}`

### `save_scene`
保存当前编辑场景。

**返回**: `{"status": "ok"}`

### `save_scene_as`
将当前编辑场景另存为新路径。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `scene_path` | string | 是 |
| `with_preview` | boolean | 否 | `false` |

**返回**: `{"status": "ok"}`

### `save_all_scenes`
保存所有已打开的场景。

**返回**: `{"status": "ok"}`

### `close_scene`
关闭当前编辑场景标签。

**返回**: `{"status": "ok"}`

### `reload_scene`
从磁盘重新加载指定场景。

| 参数 | 类型 | 必须 |
|------|------|------|
| `scene_path` | string | 是 |

**返回**: `{"status": "ok"}`

### `rename_scene`
重命名/移动场景文件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `source_path` | string | 是 |
| `dest_path` | string | 是 |

**返回**: `{"status": "ok"}` — 若目标已打开但不是活动标签，返回错误。

### `delete_scene`
删除场景文件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |

**返回**: `{"status": "ok"}`

### `is_scene_dirty`
查询当前场景是否有未保存的变更。

**返回**: `{"dirty": false}`

### `mark_scene_unsaved`
标记当前编辑场景为未保存。

**返回**: `{"status": "ok"}`

### `instantiate_scene`
实例化子场景。

| 参数 | 类型 | 必须 |
|------|------|------|
| `scene_path` | string | 是 |
| `parent_path` | string | 是 |

**返回**: `{"status": "ok", "name": "..."}`

### `get_open_scenes`
列出所有已打开的场景文件路径。

**返回**: `{"scenes": ["res://scene1.tscn", "res://scene2.tscn"]}`

### `get_open_scene_roots`
列出所有已打开场景的根节点信息。

**返回**: `{"roots": [{"path":"res://s1.tscn","type":"Node2D","name":"Root1"}]}`

### `set_node_transform_2d`
一次性设置 2D 节点 position + rotation + scale（单个 undo action）。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 否 | 0 |
| `y` | number | 否 | 0 |
| `degrees` | number | 否 | 0 |
| `scale_x` | number | 否 | 1 |
| `scale_y` | number | 否 | 1 |

**返回**: `{"status": "ok"}`

### `set_node_transform_3d`
一次性设置 3D 节点 position + rotation + scale（单个 undo action）。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 否 | 0 |
| `y` | number | 否 | 0 |
| `z` | number | 否 | 0 |
| `rot_x` | number | 否 | 0 |
| `rot_y` | number | 否 | 0 |
| `rot_z` | number | 否 | 0 |
| `scale_x` | number | 否 | 1 |
| `scale_y` | number | 否 | 1 |
| `scale_z` | number | 否 | 1 |

**返回**: `{"status": "ok"}`

---

## ScriptGdCommands (5)

### `create_gdscript`
创建 GDScript 文件。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `path` | string | 是 |
| `base_class` | string | 是 |
| `class_name` | string | 否 | 无 |
| `template` | string | 否 | 默认模板 |
| `overwrite` | boolean | 否 | `false` |

**返回**: `{"status": "ok", "path": "..."}`

### `edit_gdscript`
修改 GDScript 文件源码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `source` | string | 是 |

**返回**: `{"status": "ok"}`

### `read_gdscript`
读取 GDScript 文件源码。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `path` | string | 是 |
| `from_editor` | boolean | 否 | `false` |

**返回**: `{"source": "..."}`

### `list_gdscripts`
列出项目所有 GDScript 文件。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `root` | string | 否 | `res://` |
| `include_addons` | boolean | 否 | `false` |
| `max_results` | integer | 否 | 无限制 |

**返回**: `{"scripts": ["res://script1.gd", "res://script2.gd"]}`

### `validate_gdscript`
验证 GDScript 语法（通过 Godot LSP）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |

**返回**: `{"valid": true}` 或 `{"valid": false, "errors": [...]}`

**注意**: 需要 Editor Settings → Network → Language Server → Enable 为 ON。创建一个临时 tokio 运行时。

---

## ScriptCsCommands (6)

### `csharp_create_solution`
创建 C# Solution。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `enable_nativeaot` | boolean | 否 | `false` |

**返回**: `{"status": "ok", "sln_path": "...", "csproj_path": "..."}`

### `create_csharp_script`
创建 C# 脚本文件（需先执行 csharp_create_solution）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `base_class` | string | 是 |
| `overwrite` | boolean | 否 |

**返回**: `{"status": "ok", "path": "..."}`

### `edit_csharp_script`
修改 C# 脚本源码（需执行 csharp_build 编译）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `source` | string | 是 |

**返回**: `{"status": "ok"}`, 更新后通知 `EditorFileSystem`。

### `read_csharp_script`
读取 C# 脚本源码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |

**返回**: `{"source": "..."}`

### `list_csharp_scripts`
列出项目所有 C# 脚本文件。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `root` | string | 否 | `res://` |
| `include_addons` | boolean | 否 | `false` |
| `max_results` | integer | 否 | 无限制 |

**返回**: `{"scripts": ["res://script1.cs", "res://script2.cs"]}`

### `csharp_build`
编译 C# 项目（调用 dotnet build）。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `configuration` | string | 否 | `Debug` |

**返回**: `{"status": "ok", "output": "..."}`, 编译完成后通知 `EditorFileSystem`。

---

## SearchCommands (3)

### `find_in_file`
在单个文件中搜索文本或正则。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `path` | string | 是 |
| `pattern` | string | 是 |
| `case_sensitive` | boolean | 否 | `false` |
| `literal` | boolean | 否 | `true` |
| `max_matches` | integer | 否 | 无限制 |

**返回**: `{"matches": [{"line": 10, "content": "...", "column": 5}]}`

### `search_project`
在项目中全文搜索。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `pattern` | string | 是 |
| `root` | string | 否 | `res://` |
| `extensions` | array[string] | 否 | `["gd","tscn","cs","res"]` |
| `case_sensitive` | boolean | 否 | `false` |
| `literal` | boolean | 否 | `true` |
| `max_files` | integer | 否 | 50 |
| `max_matches` | integer | 否 | 500 |

**返回**: `{"results": [{"file":"res://main.gd","matches":[{"line":5, ...}]}]}`

### `find_and_replace`
项目级查找替换。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `path` | string | 是 |
| `pattern` | string | 是 |
| `replacement` | string | 是 |
| `case_sensitive` | boolean | 否 | `false` |
| `literal` | boolean | 否 | `true` |
| `max_replacements` | integer | 否 | 无限制 |

**返回**: `{"status": "ok", "replacements": 5}`, 修改后通知 `EditorFileSystem`。

---

## UndoCommands (2)

### `undo`
撤销最近一次操作。

**返回**: `{"status": "ok"}`

### `redo`
重做最近一次操作。

**返回**: `{"status": "ok"}`

---

## Property3dCommands (6)

### `get_node_position_3d`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"x": 0.0, "y": 0.0, "z": 0.0}`

### `set_node_position_3d`
| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 否 | 0 |
| `y` | number | 否 | 0 |
| `z` | number | 否 | 0 |

**返回**: `{"status": "ok"}`

### `get_node_rotation_3d`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"x": 0.0, "y": 0.0, "z": 0.0}`

### `set_node_rotation_3d`
| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 否 | 0 |
| `y` | number | 否 | 0 |
| `z` | number | 否 | 0 |

**返回**: `{"status": "ok"}`

### `get_node_scale_3d`
| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"x": 1.0, "y": 1.0, "z": 1.0}`

### `set_node_scale_3d`
| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 否 | 1 |
| `y` | number | 否 | 1 |
| `z` | number | 否 | 1 |

**返回**: `{"status": "ok"}`

---

## NodeConvenience (4)

### `set_node_texture`
设置节点纹理。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `texture_path` | string | 是 |
| `property` | string | 否 | `"texture"` |

**返回**: `{"status": "ok"}`, 支持 `TextureButton` 的 `texture_normal/pressed/hover` 等多种属性。

### `get_node_texture`
获取节点纹理。

| 参数 | 类型 | 必须 | 默认 |
|------|------|------|------|
| `node_path` | string | 是 |
| `property` | string | 否 | `"texture"` |

**返回**: `{"path": "res://icon.svg"}`

### `set_node_collision_layer`
设置碰撞层。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `layer` | integer | 是 |

**返回**: `{"status": "ok"}` — 非 CollisionObject2D/3D 节点返回错误。

### `get_node_collision_layer`
获取碰撞层。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"layer": 1}`

### `set_node_collision_mask`
设置碰撞掩码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `mask` | integer | 是 |

**返回**: `{"status": "ok"}` — 非 CollisionObject2D/3D 节点返回错误。

### `get_node_collision_mask`
获取碰撞掩码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

**返回**: `{"mask": 1}`

---

## SceneInfo (1)

### `get_open_scene_roots`
列出所有已打开场景的根节点信息。

**返回**: `{"roots": [...]}`
