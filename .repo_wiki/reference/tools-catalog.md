# 工具目录

> 全部 124 个 ITool 的描述、参数和返回值，按 category 分组共 17 组。通过 `// @tool register` 注释 + `tools/codegen.py` 自动注册。

## 分组速览

| 组 | 计数 | 映射后路径 |
|----|------|-----------|
| [MetaTools](#metatools-5) | 5 | `other/meta` |
| [NodeTools](#nodetools-21) | 21 | `node/operation` |
| [PropertyTools](#propertytools-21) | 21 | `node/property/2d` |
| [CollisionTools](#collisiontools-2) | 2 | `node/collision` |
| [FindTools](#findtools-4) | 4 | `node/find` |
| [ScriptHelpersTools](#scripthelperstools-3) | 3 | `script/helpers` |
| [ProjectSettingsTools](#projectsettingstools-7) | 7 | `settings/project` |
| [SceneTools](#scenetools-16) | 16 | `scene` |
| [ScriptGdTools](#scriptgdtools-5) | 5 | `script/gdscript` |
| [ScriptCsTools](#scriptcstools-6) | 6 | `script/csharp` |
| [SearchTools](#searchtools-3) | 3 | `editor/search` |
| [UndoTools](#undotools-2) | 2 | `other/undo` |
| [Property3dTools](#property3dtools-6) | 6 | `node/property/3d` |
| [EditorControlTools](#editorcontroltools-7) | 7 | `editor/general` |
| [ProjectSettingsExtTools](#projectsettingsexttools-10) | 10 | `settings/extended` |
| [PluginManagementTools](#pluginmanagementtools-2) | 2 | `other/plugin_management` |
| [InputMapTools](#inputmaptools-4) | 4 | `settings/input_map` |
| **总计** | **124** | |

> **注**：ScriptCsTools 已激活，但 `csharp_build` 需要 `dotnet` CLI 在 PATH 上。

---

## MetaTools (5)

始终可见的元工具（渐进式披露第一层），位于 `tools/meta/`。

### `godot_info`
返回连接状态与引擎/插件/项目/编辑器信息。合并了旧 `ping`、`get_engine_version`、`get_plugin_version` 的功能。

**返回**: `{"connection": {"status": "ok"}, "engine": {"version": "4.6.0", "hash": "..."}, "project": {...}, "editor": {...}}`

### `list_tool_categories`
返回嵌套的分类树（顶级 + 子分类），每个分类包含名称、标签、描述、工具计数。

**返回**: `[{"name": "node", "label": "Node", "description": "...", "tool_count": N, "total": N, "subcategories": [...]}]`

### `list_tools`
根据分类名称返回该分类下所有工具的简短信息列表。

| 参数 | 类型 | 必须 |
|------|------|------|
| `category` | string | 是 |
| `filter` | string | 否 |

### `get_tool_schema`
返回指定工具的完整元数据（名称、描述、简介、分类、来源、输入 Schema）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `tool_name` | string | 是 |

### `call_tool`
通过工具名称调用任意已注册工具（原生工具调用的兜底方法）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `tool_name` | string | 是 |
| `arguments` | object | 否 |

---

## NodeTools (21)

位于 `tools/node/`，覆盖节点 CRUD、属性读写、脚本挂载、分组管理和 transform 设置：
`get_scene_tree`, `get_node_path`, `get_property_list`, `get_property`, `create_node`, `delete_node`, `rename_node`, `set_property`, `duplicate_node`, `move_node`, `attach_script`, `detach_script`, `reset_parent`, `set_as_root`, `batch_set_property`, `add_node_to_group`, `remove_node_from_group`, `set_node_transform_2d`, `set_node_transform_3d`, `get_node_info`, `get_script_variables`

---

## PropertyTools (21)

### `get_node_position`
获取节点位置。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_position`
设置节点位置。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 是 |
| `y` | number | 是 |

### `get_node_rotation`
获取节点旋转角度（度）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_rotation`
设置节点旋转角度（度）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `degrees` | number | 是 |

### `get_node_scale`
获取节点缩放。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_scale`
设置节点缩放。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `x` | number | 是 |
| `y` | number | 是 |

### `get_node_visible`
获取节点可见性。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_visible`
设置节点可见性。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `visible` | boolean | 是 |

### `get_node_modulate`
获取节点调制颜色。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_modulate`
设置节点调制颜色。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `r`, `g`, `b`, `a` | number | 否（不设保持原值） |

### `get_node_z_index`
获取节点 Z 轴索引。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_z_index`
设置节点 Z 轴索引。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `z_index` | integer | 是 |

### `get_node_text`
获取节点文本 (Label 等)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_text`
设置节点文本 (Label 等)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `text` | string | 是 |

### `get_node_collision_layer`
获取碰撞层。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_collision_layer`
设置碰撞层。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `layer` | integer | 是 |

### `get_node_collision_mask`
获取碰撞掩码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |

### `set_node_collision_mask`
设置碰撞掩码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `mask` | integer | 是 |

### `set_node_unique_name`
设置节点唯一名称（%前缀）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `unique` | boolean | 否 |

### `get_node_texture`
获取节点纹理属性 (Texture2D)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `property` | string | 否（默认 `"texture"`） |

### `set_node_texture`
设置节点纹理属性 (Texture2D)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `texture_path` | string | 是 |
| `property` | string | 否（默认 `"texture"`） |

**注意**: 非 CollisionObject2D/3D 节点调用碰撞层/掩码工具会报错。`TextureButton` 支持 `texture_normal/pressed/hover/disabled/focused` 属性；`TextureProgressBar` 支持 `texture_under/over/progress`。`set_node_position`/`set_node_rotation`/`set_node_scale` 的 x/y 参数不设置为保持原值。

---

## CollisionTools (2)

### `add_circle_collision`
为节点添加圆形碰撞体 (CollisionShape2D + CircleShape2D)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `radius` | number | 否 |

### `add_rectangle_collision`
为节点添加矩形碰撞体 (CollisionShape2D + RectangleShape2D)。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `width` | number | 否 |
| `height` | number | 否 |

**注意**: 检测已有的 `CollisionShape2D`；`mode` 字段指示操作路径。

---

## FindTools (4)

### `find_nodes_by_name`
按名称子串搜索节点（大小写敏感，包含匹配，不是 glob）。

### `find_nodes_by_type`
按节点类型精确搜索。

### `find_nodes_by_group`
按组名搜索节点。

### `find_nodes_by_script`
按脚本路径搜索节点。

通用参数：

| 参数 | 类型 | 必须 |
|------|------|------|
| `pattern` / `node_class` / `group` / `script_path` | string | 是 |
| `max_results` | integer | 否 |

---

## ScriptHelpersTools (3)

### `call_method`
调用节点方法。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `method` | string | 是 |
| `args` | array | 否 |

### `get_variable`
读取节点脚本变量（编辑器模式下仅支持 @export 变量）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `variable` | string | 是 |

### `set_variable`
写入节点脚本变量（编辑器模式下仅支持 @export 变量）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `node_path` | string | 是 |
| `variable` | string | 是 |
| `value` | any | 是 |

**注意**: 非导出变量使用 `PlaceHolderScriptInstance`。

---

## ProjectSettingsTools (7)

### `get_project_setting`
读取项目设置。

| 参数 | 类型 | 必须 |
|------|------|------|
| `property` | string | 是 |

### `set_project_setting`
写入项目设置。

| 参数 | 类型 | 必须 |
|------|------|------|
| `property` | string | 是 |
| `value` | any | 是 |

### `set_main_scene`
设置主场景。

| 参数 | 类型 | 必须 |
|------|------|------|
| `scene_path` | string | 是 |

### `list_autoloads`
列出项目所有 Autoload 单例。

### `add_autoload`
添加 Autoload 单例。

| 参数 | 类型 | 必须 |
|------|------|------|
| `name` | string | 是 |
| `path` | string | 是 |

### `remove_autoload`
移除 Autoload 单例。

| 参数 | 类型 | 必须 |
|------|------|------|
| `name` | string | 是 |

### `list_scenes`
列出项目中所有场景文件 (.tscn/.scn)。

---

## SceneTools (16)

包含 `create_scene`, `delete_scene`, `rename_scene`, `branch_to_scene`, `scene_to_branch`, `instantiate_scene`, `open_scene`, `close_scene`, `save_scene`, `save_scene_as`, `save_all_scenes`, `reload_scene`, `get_open_scenes`, `get_open_scene_roots`, `mark_scene_unsaved`, `is_scene_dirty`。

所有场景文件操作必须使用 `EditorInterface` API——编辑器看不到直接的 `.tscn` 文件写入。写入文件后需调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()` 让编辑器感知变更。

---

## ScriptGdTools (5)

### `create_gdscript`
创建 GDScript 文件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `base_class` | string | 是（默认 `RefCounted`） |
| `class_name` | string | 否 |
| `template` | string | 否 |
| `overwrite` | boolean | 否 |

### `read_gdscript`
读取 GDScript 文件源码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `from_editor` | boolean | 否 |

### `edit_gdscript`
修改 GDScript 文件源码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `source` | string | 是 |

### `validate_gdscript`
验证 GDScript 语法（通过 Godot LSP）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |

**注意**: 需要编辑器设置 → 网络 → Language Server → 启用。通过 `StreamPeerTCP` 同步连接 LSP 服务器，不创建额外线程。

### `list_gdscripts`
列出项目所有 GDScript 文件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `root` | string | 否 |
| `include_addons` | boolean | 否 |
| `max_results` | integer | 否 |

---

## ScriptCsTools (6)

> C# 工具在 `register_all_tools()` 中通过 `register_itools()` 注册，均已激活。`csharp_build` 需要 `dotnet` CLI 在 PATH 上。

### `create_csharp_script`
创建 C# 脚本文件（需先执行 `csharp_create_solution`）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `base_class` | string | 是 |
| `overwrite` | boolean | 否 |

### `read_csharp_script`
读取 C# 脚本文件源码。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |

### `edit_csharp_script`
修改 C# 脚本文件源码（需执行 `csharp_build` 编译）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `source` | string | 是 |

### `list_csharp_scripts`
列出项目所有 C# 脚本文件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `root` | string | 否 |
| `include_addons` | boolean | 否 |
| `max_results` | integer | 否 |

### `csharp_build`
编译 C# 项目（调用 `dotnet build`）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `configuration` | string | 否 |

### `csharp_create_solution`
创建 C# Solution 文件。

---

## SearchTools (3)

### `find_in_file`
在单个文件中搜索文本或正则。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `pattern` | string | 是 |
| `literal` | boolean | 否 |
| `case_sensitive` | boolean | 否 |
| `max_matches` | integer | 否 |

### `search_project`
在项目中全文搜索（递归）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `pattern` | string | 是 |
| `literal` | boolean | 否 |
| `extensions` | array | 否 |
| `root` | string | 否 |
| `case_sensitive` | boolean | 否 |
| `max_matches` | integer | 否 |
| `max_files` | integer | 否 |

### `find_and_replace`
项目级查找替换（脚本/tscn 文件中的文本）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `path` | string | 是 |
| `pattern` | string | 是 |
| `replacement` | string | 是 |
| `literal` | boolean | 否 |
| `case_sensitive` | boolean | 否 |
| `max_replacements` | integer | 否 |

---

## UndoTools (2)

### `undo`
撤销最近一次操作。

### `redo`
重做最近一次操作。

---

## Property3dTools (6)

### `get_node_position_3d`
获取 Node3D 节点的 Vector3 位置。

### `set_node_position_3d`
设置 Node3D 节点的 Vector3 位置。

### `get_node_rotation_3d`
获取 Node3D 节点的 Euler 角旋转（度数）。

### `set_node_rotation_3d`
设置 Node3D 节点的 Euler 角旋转（度数）。

### `get_node_scale_3d`
获取 Node3D 节点的 Vector3 缩放。

### `set_node_scale_3d`
设置 Node3D 节点的 Vector3 缩放。

---

## EditorControlTools (7)

### `play_current_scene`
播放当前编辑器中打开的场景。

### `play_main_scene`
播放项目主场景。

### `stop_scene`
停止正在运行的场景。

### `is_scene_playing`
查询场景是否正在播放。

### `refresh_filesystem`
刷新编辑器文件系统扫描。

### `godot_editor_restart`
重启 Godot 编辑器（通过 `OS::create_process()` 启动新进程后退出）。

### `get_editor_info`
获取编辑器信息：引擎版本、编辑器路径、缩放比例、语言。

---

## ProjectSettingsExtTools (10)

### `get_display_settings`
获取显示/窗口相关设置（分辨率、拉伸、全屏模式等）。

### `set_display_settings`
设置显示/窗口相关设置。

### `get_project_info`
获取项目基本信息（名称、描述、版本、图标、主场景等）。

### `set_project_info`
设置项目基本信息。

### `get_physics_settings`
获取物理引擎设置（2D/3D 重力、阻尼、帧率等）。

### `set_physics_settings`
设置物理引擎参数。

### `get_rendering_settings`
获取渲染相关设置（渲染器、抗锯齿、像素对齐等）。

### `set_rendering_settings`
设置渲染参数。

### `get_layer_names`
获取物理/导航/渲染层名称（按分类）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `category` | string | 是（`2d_physics` / `2d_navigation` / `2d_render` / `3d_physics` / `3d_navigation` / `3d_render` / `avoidance`） |

### `set_layer_names`
设置物理/导航/渲染层名称。

| 参数 | 类型 | 必须 |
|------|------|------|
| `category` | string | 是 |
| `layers` | object | 是（`{"1":"Player","2":"Enemy"}`） |

---

## PluginManagementTools (2)

### `list_plugins`
列出 `res://addons/` 中所有插件及其启用状态。

### `set_plugin_enabled`
启用或禁用编辑器插件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `plugin` | string | 是 |
| `enabled` | boolean | 是 |

---

## InputMapTools (4)

### `list_input_actions`
列出输入动作及其绑定的事件。

| 参数 | 类型 | 必须 |
|------|------|------|
| `include_builtin` | boolean | 否（默认 false，是否包含 `ui_*` 内置动作） |

### `add_input_action`
添加新的输入动作。

| 参数 | 类型 | 必须 |
|------|------|------|
| `name` | string | 是 |
| `deadzone` | number | 否（默认 0.5） |

### `set_input_action_events`
设置输入动作绑定的事件（替换/追加/清空）。

| 参数 | 类型 | 必须 |
|------|------|------|
| `name` | string | 是 |
| `mode` | string | 否（`replace` / `add` / `clear`） |
| `events` | array | 否 |

### `remove_input_action`
移除输入动作。

| 参数 | 类型 | 必须 |
|------|------|------|
| `name` | string | 是 |
