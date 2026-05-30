# 工具目录

GodotMCP 提供 **122 个编辑器操作工具**，分布在 16 个活跃注册组中。以下是按类别整理的完整列表。

## 元操作（3）

| 工具 | 说明 |
|------|------|
| `ping` | 检测与 Godot 编辑器的连接状态 |
| `get_engine_version` | 获取 Godot 引擎版本号 |
| `get_plugin_version` | 获取 GodotMCP 插件版本号 |

## 场景管理（16）

| 工具 | 说明 |
|------|------|
| `create_scene` | 创建新场景文件 |
| `open_scene` | 在编辑器中打开场景文件 |
| `save_scene` | 保存当前编辑场景 |
| `save_scene_as` | 当前场景另存为新路径 |
| `save_all_scenes` | 保存所有已打开场景 |
| `close_scene` | 关闭当前编辑场景标签 |
| `reload_scene` | 从磁盘重新加载指定场景 |
| `delete_scene` | 删除场景文件 |
| `rename_scene` | 重命名/移动场景文件 |
| `branch_to_scene` | 将节点分支转为场景文件 |
| `scene_to_branch` | 将实例化场景转为本地分支 |
| `instantiate_scene` | 实例化子场景 |
| `get_open_scenes` | 列出所有已打开的场景文件路径 |
| `get_open_scene_roots` | 列出所有已打开场景的根节点信息 |
| `mark_scene_unsaved` | 标记当前场景为未保存 |
| `is_scene_dirty` | 查询当前场景是否有未保存变更 |

## 节点操作（21）

| 工具 | 说明 |
|------|------|
| `create_node` | 创建新节点 |
| `delete_node` | 删除节点 |
| `rename_node` | 重命名节点 |
| `duplicate_node` | 复制节点 |
| `move_node` | 移动节点到新父节点 |
| `reset_parent` | 重置父节点（reparent） |
| `set_as_root` | 设置节点为场景根 |
| `get_scene_tree` | 获取当前场景节点树 |
| `get_node_path` | 获取节点路径 |
| `get_node_info` | 获取节点完整信息（类型、脚本、可见性、组等） |
| `get_property_list` | 获取节点所有属性列表 |
| `get_property` | 获取节点指定属性值 |
| `set_property` | 修改节点属性值 |
| `batch_set_property` | 批量修改多个节点同名属性 |
| `attach_script` | 为节点挂载脚本 |
| `detach_script` | 卸载节点脚本 |
| `add_node_to_group` | 添加节点到组 |
| `remove_node_from_group` | 从组中移除节点 |
| `set_node_transform_2d` | 一次性设置 2D position + rotation + scale |
| `set_node_transform_3d` | 一次性设置 3D position + rotation + scale |
| `get_script_variables` | 列出脚本上的所有 @export 变量 |

## 2D 属性（21）

| 工具 | 说明 |
|------|------|
| `get_node_position` | 获取节点 2D 位置 |
| `set_node_position` | 设置节点 2D 位置 |
| `get_node_rotation` | 获取节点旋转角度（度） |
| `set_node_rotation` | 设置节点旋转角度（度） |
| `get_node_scale` | 获取节点缩放 |
| `set_node_scale` | 设置节点缩放 |
| `get_node_visible` | 获取节点可见性 |
| `set_node_visible` | 设置节点可见性 |
| `get_node_modulate` | 获取节点调制颜色 |
| `set_node_modulate` | 设置节点调制颜色 |
| `get_node_z_index` | 获取节点 Z 轴索引 |
| `set_node_z_index` | 设置节点 Z 轴索引 |
| `get_node_text` | 获取节点文本 |
| `set_node_text` | 设置节点文本 |
| `get_node_collision_layer` | 获取碰撞层 |
| `set_node_collision_layer` | 设置碰撞层 |
| `get_node_collision_mask` | 获取碰撞掩码 |
| `set_node_collision_mask` | 设置碰撞掩码 |
| `get_node_texture` | 获取节点纹理属性 |
| `set_node_texture` | 设置节点纹理属性 |
| `set_node_unique_name` | 设置节点唯一名称（%前缀） |

## 3D 属性（6）

| 工具 | 说明 |
|------|------|
| `get_node_position_3d` | 获取 Node3D 位置 |
| `set_node_position_3d` | 设置 Node3D 位置 |
| `get_node_rotation_3d` | 获取 Euler 角旋转（度） |
| `set_node_rotation_3d` | 设置 Euler 角旋转（度） |
| `get_node_scale_3d` | 获取 Node3D 缩放 |
| `set_node_scale_3d` | 设置 Node3D 缩放 |

## 碰撞体（2）

| 工具 | 说明 |
|------|------|
| `add_circle_collision` | 添加圆形碰撞体（CollisionShape2D + CircleShape2D） |
| `add_rectangle_collision` | 添加矩形碰撞体（CollisionShape2D + RectangleShape2D） |

## 节点搜索（4）

| 工具 | 说明 |
|------|------|
| `find_nodes_by_name` | 按名称子串搜索节点 |
| `find_nodes_by_type` | 按节点类型精确搜索 |
| `find_nodes_by_group` | 按组名搜索节点 |
| `find_nodes_by_script` | 按脚本路径搜索节点 |

## GDScript（5）

| 工具 | 说明 |
|------|------|
| `create_gdscript` | 创建 GDScript 文件 |
| `read_gdscript` | 读取 GDScript 文件源码 |
| `edit_gdscript` | 修改 GDScript 文件源码 |
| `validate_gdscript` | 通过 Godot LSP 验证 GDScript 语法 |
| `list_gdscripts` | 列出项目所有 GDScript 文件 |

## C# 脚本（6，暂未注册）

| 工具 | 说明 |
|------|------|
| `csharp_create_solution` | 创建 C# Solution 文件 |
| `csharp_build` | 编译 C# 项目 |
| `create_csharp_script` | 创建 C# 脚本文件 |
| `read_csharp_script` | 读取 C# 脚本文件源码 |
| `edit_csharp_script` | 修改 C# 脚本文件源码 |
| `list_csharp_scripts` | 列出项目所有 C# 脚本文件 |

## 脚本辅助（3）

| 工具 | 说明 |
|------|------|
| `call_method` | 调用节点方法 |
| `get_variable` | 读取脚本 @export 变量值 |
| `set_variable` | 写入脚本 @export 变量值 |

## 项目设置（7）

| 工具 | 说明 |
|------|------|
| `get_project_setting` | 读取项目设置 |
| `set_project_setting` | 写入项目设置 |
| `set_main_scene` | 设置主场景 |
| `list_autoloads` | 列出项目所有 Autoload 单例 |
| `add_autoload` | 添加 Autoload 单例 |
| `remove_autoload` | 移除 Autoload 单例 |
| `list_scenes` | 列出项目中所有场景文件 |

## 项目设置扩展（10）

| 工具 | 说明 |
|------|------|
| `get_display_settings` | 获取显示/窗口相关设置 |
| `set_display_settings` | 设置显示/窗口相关设置 |
| `get_project_info` | 获取项目基本信息 |
| `set_project_info` | 设置项目基本信息 |
| `get_physics_settings` | 获取物理引擎设置 |
| `set_physics_settings` | 设置物理引擎参数 |
| `get_rendering_settings` | 获取渲染相关设置 |
| `set_rendering_settings` | 设置渲染参数 |
| `get_layer_names` | 获取物理/导航/渲染层名称 |
| `set_layer_names` | 设置物理/导航/渲染层名称 |

## 编辑器控制（7）

| 工具 | 说明 |
|------|------|
| `get_editor_info` | 获取编辑器信息（引擎版本、路径、缩放、语言） |
| `play_current_scene` | 播放当前场景 |
| `play_main_scene` | 播放项目主场景 |
| `stop_scene` | 停止运行中的场景 |
| `is_scene_playing` | 查询场景是否正在播放 |
| `refresh_filesystem` | 刷新编辑器文件系统扫描 |
| `godot_editor_restart` | 重启 Godot 编辑器 |

## 输入映射（4）

| 工具 | 说明 |
|------|------|
| `list_input_actions` | 列出输入动作及其绑定事件 |
| `add_input_action` | 添加新的输入动作 |
| `remove_input_action` | 移除输入动作 |
| `set_input_action_events` | 设置输入动作绑定的事件 |

## 搜索（3）

| 工具 | 说明 |
|------|------|
| `find_in_file` | 在单个文件中搜索文本或正则 |
| `search_project` | 项目级全文搜索 |
| `find_and_replace` | 项目级查找替换 |

## 插件管理（2）

| 工具 | 说明 |
|------|------|
| `list_plugins` | 列出所有插件及其启用状态 |
| `set_plugin_enabled` | 启用或禁用编辑器插件 |

## Undo/Redo（2）

| 工具 | 说明 |
|------|------|
| `undo` | 撤销最近一次操作 |
| `redo` | 重做最近一次操作 |
