# 工具目录

GodotMCP 提供 **149+ 编辑器操作工具**，按功能域分为元操作、场景树、脚本、文件系统、工作区/调试器、运行时桥接等类别。工具通过 X-macro 自动注册，支持 `find_tool` 搜索引擎和渐进式发现。

## 元操作（6）

| 工具 | 说明 |
|------|------|
| `get_info` | 获取编辑器连接状态与运行环境信息 |
| `get_tools` | 列出所有已注册工具 |
| `get_categories` | 列出工具分类树 |
| `get_tool_detail` | 获取指定工具的完整元数据 |
| `find_tool` | 全文搜索工具（名称、描述、分类） |
| `call_tool` | 通过名称调用任意工具 |

## 信号管理（4）

| 工具 | 说明 |
|------|------|
| `connect_signal` | 连接节点信号 |
| `disconnect_signal` | 断开节点信号连接 |
| `list_signals` | 列出节点所有信号 |
| `get_signal_connections` | 获取信号连接列表 |

## 节点组（4）

| 工具 | 说明 |
|------|------|
| `add_to_group` | 添加节点到组 |
| `remove_from_group` | 从组中移除节点 |
| `get_node_groups` | 获取节点所属的组 |
| `get_nodes_in_group` | 获取组内所有节点 |

## 资源操作（7）

| 工具 | 说明 |
|------|------|
| `save_resource` | 保存资源到文件 |
| `load_resource` | 从路径加载资源 |
| `new_resource` | 创建新资源 |
| `duplicate_resource` | 复制资源 |
| `clear_resource` | 清除节点资源引用 |
| `get_resource_info` | 获取资源信息 |

## 属性兜底（2）

| 工具 | 说明 |
|------|------|
| `get_node_property` | 获取任意节点属性值（通用兜底） |
| `set_node_property` | 设置任意节点属性值（通用兜底） |

## 场景树操作（24）

| 工具 | 说明 |
|------|------|
| `add_node` | 创建新节点 |
| `delete_node` | 删除节点 |
| `rename_node` | 重命名节点 |
| `move_node` | 移动节点到新父节点 |
| `duplicate_node` | 复制节点 |
| `copy_node` | 复制节点到剪贴板 |
| `paste_node` | 粘贴剪贴板节点 |
| `cut_node` | 剪切节点 |
| `get_scene_tree` | 获取当前场景节点树 |
| `save_scene` | 保存当前场景 |
| `new_scene` | 创建新场景 |
| `change_node_type` | 更改节点类型 |
| `attach_script` | 为节点挂载脚本 |
| `detach_script` | 卸载节点脚本 |
| `reparent_node` | 重新设置父节点 |
| `reparent_to_new_node` | 重设父节点到新建节点 |
| `group_selected_nodes` | 将选中节点归组 |
| `make_local` | 将实例化场景节点本地化 |
| `save_branch_as_scene` | 将节点分支保存为场景文件 |
| `instance_child_scene` | 实例化子场景 |
| `set_unique_name` | 设置节点唯一名称（%前缀） |
| `toggle_editable_children` | 切换子节点可编辑状态 |
| `toggle_edit_group` | 切换编辑组状态 |
| `toggle_placeholder` | 切换占位符模式 |

## 动画（5）

| 工具 | 说明 |
|------|------|
| `create_animation_player` | 创建 AnimationPlayer 节点 |
| `create_animation_clip` | 创建动画剪辑 |
| `add_animation_track` | 添加动画轨道 |
| `set_keyframe` | 设置关键帧 |
| `get_animation_info` | 获取动画信息 |

## UI 控件（4）

| 工具 | 说明 |
|------|------|
| `create_control` | 创建 UI 控件节点 |
| `create_stylebox` | 创建 StyleBox 资源 |
| `set_layout_preset` | 设置控件布局预设 |
| `set_theme_override` | 设置主题覆盖 |

## 碰撞体（1）

| 工具 | 说明 |
|------|------|
| `create_collision_shape` | 创建碰撞形状 |

## ClassDB 文档查询（8）

| 工具 | 说明 |
|------|------|
| `search_docs` | 搜索 Godot 文档 |
| `get_class_info` | 获取类的完整信息 |
| `get_best_practices` | 获取最佳实践建议 |
| `get_class_list` | 列出所有 Godot 类 |
| `get_inheritance_chain` | 获取类继承链 |
| `get_property_doc` | 查询属性文档 |
| `get_method_doc` | 查询方法文档 |
| `get_enum_doc` | 查询枚举文档 |

## 导出（2）

| 工具 | 说明 |
|------|------|
| `list_export_presets` | 列出导出预设 |
| `export_project` | 导出项目 |

## 文件系统（12）

| 工具 | 说明 |
|------|------|
| `create` | 创建文件 |
| `create_directory` | 创建目录 |
| `create_scene` | 创建场景文件 |
| `create_resource` | 创建资源文件 |
| `create_gdshader` | 创建着色器文件 |
| `delete_file` | 删除文件 |
| `move_file` | 移动/重命名文件 |
| `copy_file` | 复制文件 |
| `open_file` | 在编辑器中打开文件 |
| `list_directory` | 列出目录内容 |
| `search_files` | 搜索项目文件 |
| `save_resource_as` | 另存资源 |

## 输入映射（1）

| 工具 | 说明 |
|------|------|
| `input_list_actions` | 列出所有输入动作及绑定事件 |

## 插件管理（3）

| 工具 | 说明 |
|------|------|
| `list_plugins` | 列出所有插件及启用状态 |
| `enable_plugin` | 启用插件 |
| `disable_plugin` | 禁用插件 |

## 项目脚手架（1）

| 工具 | 说明 |
|------|------|
| `create_project` | 创建新的 Godot 项目 |

## 脚本工具（12）

| 工具 | 说明 |
|------|------|
| `read_gd_script` | 读取 GDScript 文件 |
| `write_gd_script` | 写入 GDScript 文件 |
| `patch_gd_script` | 局部修改 GDScript 文件 |
| `validate_gd_script` | 验证 GDScript 语法（通过 LSP） |
| `list_gd_scripts` | 列出项目所有 GDScript |
| `grep_scripts` | 在脚本中搜索文本 |
| `glob_scripts` | 按模式搜索脚本文件 |
| `read_csharp_script` | 读取 C# 脚本文件 |
| `write_csharp_script` | 写入 C# 脚本文件 |
| `patch_csharp_script` | 局部修改 C# 脚本文件 |
| `validate_csharp_script` | 验证 C# 脚本语法 |
| `list_csharp_scripts` | 列出项目所有 C# 脚本 |

## 项目设置（4）

| 工具 | 说明 |
|------|------|
| `get_setting` | 读取项目设置 |
| `set_setting` | 写入项目设置 |
| `reset_setting` | 重置项目设置为默认值 |
| `list_settings` | 列出所有项目设置 |

## 着色器（3）

| 工具 | 说明 |
|------|------|
| `create_shader` | 创建着色器 |
| `read_shader` | 读取着色器源码 |
| `apply_shader_preset` | 应用着色器预设 |

## TileMap（3）

| 工具 | 说明 |
|------|------|
| `get_tilemap_info` | 获取 TileMap 信息 |
| `set_tilemap_cells` | 设置 TileMap 单元格 |
| `erase_tilemap_cells` | 擦除 TileMap 单元格 |

## 项目图可视化（1）

| 工具 | 说明 |
|------|------|
| `get_project_graph` | 获取项目依赖关系图 |

## 工作区与调试器（31）

### 视口截图（2）

| 工具 | 说明 |
|------|------|
| `capture_viewport` | 截取编辑器视口 |
| `capture_game_viewport` | 截取游戏视口 |

### 控制台（4）

| 工具 | 说明 |
|------|------|
| `clear_console` | 清空控制台输出 |
| `get_console_output` | 获取控制台输出 |
| `get_console_errors` | 获取控制台错误 |
| `get_console_warnings` | 获取控制台警告 |

### 调试器（15）

| 工具 | 说明 |
|------|------|
| `get_debugger_state` | 获取调试器状态 |
| `get_debugger_status` | 获取调试器运行状态 |
| `get_debugger_errors` | 获取调试器错误列表 |
| `get_fps` | 获取当前 FPS |
| `get_memory_usage` | 获取内存使用量 |
| `get_object_count` | 获取对象计数 |
| `get_performance_monitors` | 获取性能监控数据 |
| `get_physics_stats` | 获取物理统计信息 |
| `get_render_stats` | 获取渲染统计信息 |
| `get_stack_trace` | 获取调用栈 |
| `get_locals` | 获取局部变量 |
| `debugger_break` | 暂停执行（断点） |
| `debugger_continue` | 继续执行 |
| `debugger_control` | 调试器控制（播放/暂停/步进） |
| `debugger_step_into` | 步入 |
| `debugger_step_out` | 步出 |
| `debugger_step_over` | 步过 |

### 断点（3）

| 工具 | 说明 |
|------|------|
| `list_breakpoints` | 列出所有断点 |
| `set_breakpoint` | 设置断点 |
| `remove_breakpoint` | 移除断点 |

### 工作区切换（5）

| 工具 | 说明 |
|------|------|
| `set_workspace` | 切换工作区（2D/3D/脚本/资产库） |
| `set_workspace_2d` | 切换到 2D 工作区 |
| `set_workspace_3d` | 切换到 3D 工作区 |
| `set_workspace_script` | 切换到脚本工作区 |
| `set_workspace_assetlib` | 切换到资产库工作区 |

## 运行时桥接（6）

| 工具 | 说明 |
|------|------|
| `get_game_scene_tree` | 获取游戏运行时场景树 |
| `get_game_node_property` | 获取游戏运行时节点属性 |
| `set_game_node_property` | 设置游戏运行时节点属性 |
| `call_method_in_game` | 调用游戏运行时方法 |
| `capture_game_screenshot` | 截取游戏运行时画面 |
| `simulate_game_input` | 模拟游戏运行时输入 |

## 运行时生命周期（6）

| 工具 | 说明 |
|------|------|
| `run_project` | 运行项目 |
| `run_current_scene` | 运行当前场景 |
| `run_specific_scene` | 运行指定场景 |
| `stop_project` | 停止运行 |
| `pause_project` | 暂停/恢复运行 |
| `set_movie_maker` | 切换 Movie Maker 模式 |
