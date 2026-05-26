# Auto-generated from crates/server/src/tool_registry.rs
# Run: python tools/extract_schemas.py --write
# flake8: noqa

from typing import Any

from godot_mcp_server.protocol import ToolInfo, ToolListUpdate, ToolState


class ToolRegistry:
    """Registry of all tool schemas, matching Rust tool_registry.rs."""

    _TOOLS: list[tuple[str, str, dict[str, Any]]] = [
        ("ping", "检测与 Godot 编辑器的连接状态（编辑器刚启动时可能需要等待几秒后再调用）", {"type": "object", "properties": {}, "required": []}),
        ("get_engine_version", "获取 Godot 引擎版本号", {"type": "object", "properties": {}, "required": []}),
        ("get_plugin_version", "获取 Godot MCP 插件版本号", {"type": "object", "properties": {}, "required": []}),
        ("get_server_version", "获取 godot-mcp-server 版本号", {"type": "object", "properties": {}, "required": []}),
        ("get_scene_tree", "获取当前场景节点树", {"type": "object", "properties": {"max_depth": {"type": "integer"}}, "required": []}),
        ("get_node_path", "获取节点路径", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("get_property_list", "获取节点所有属性列表", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("get_property", "获取节点指定属性值", {"type": "object", "properties": {}, "required": []}),
        ("create_node", "创建新节点", {"type": "object", "properties": {}, "required": []}),
        ("delete_node", "删除节点", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("rename_node", "重命名节点", {"type": "object", "properties": {}, "required": []}),
        ("set_property", "修改节点属性值 (通用兜底)", {"type": "object", "properties": {}, "required": []}),
        ("duplicate_node", "复制节点", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("move_node", "移动节点到新父节点", {"type": "object", "properties": {}, "required": []}),
        ("attach_script", "为节点挂载脚本", {"type": "object", "properties": {}, "required": []}),
        ("detach_script", "卸载节点脚本", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("reset_parent", "重设父节点 (reparent)", {"type": "object", "properties": {}, "required": []}),
        ("set_as_root", "设置节点为场景根", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("batch_set_property", "批量修改多个节点同名属性", {"type": "object", "properties": {}, "required": []}),
        ("add_node_to_group", "添加节点到组", {"type": "object", "properties": {}, "required": []}),
        ("remove_node_from_group", "从组中移除节点", {"type": "object", "properties": {}, "required": []}),
        ("get_node_position", "获取节点位置", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_position", "设置节点位置", {"type": "object", "properties": {}, "required": []}),
        ("get_node_rotation", "获取节点旋转角度 (度)", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_rotation", "设置节点旋转角度 (度)", {"type": "object", "properties": {}, "required": []}),
        ("get_node_scale", "获取节点缩放", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_scale", "设置节点缩放", {"type": "object", "properties": {}, "required": []}),
        ("get_node_visible", "获取节点可见性", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_visible", "设置节点可见性", {"type": "object", "properties": {}, "required": []}),
        ("get_node_modulate", "获取节点调制颜色", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_modulate", "设置节点调制颜色", {"type": "object", "properties": {}, "required": []}),
        ("get_node_z_index", "获取节点 Z 轴索引", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_z_index", "设置节点 Z 轴索引", {"type": "object", "properties": {}, "required": []}),
        ("get_node_text", "获取节点文本 (Label 等)", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_text", "设置节点文本 (Label 等)", {"type": "object", "properties": {}, "required": []}),
        ("get_node_collision_layer", "获取碰撞层", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_collision_layer", "设置碰撞层", {"type": "object", "properties": {}, "required": []}),
        ("get_node_collision_mask", "获取碰撞掩码", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_collision_mask", "设置碰撞掩码", {"type": "object", "properties": {}, "required": []}),
        ("set_node_unique_name", "设置节点唯一名称 (%前缀)", {"type": "object", "properties": {}, "required": []}),
        ("get_node_texture", "获取节点纹理属性 (Texture2D)。通过 property 参数指定属性名，默认 texture。TextureButton 支持 texture_normal/pressed/hover/disabled/focused；TextureProgressBar 支持 texture_under/over/progress", {"type": "object", "properties": {}, "required": []}),
        ("set_node_texture", "设置节点纹理属性 (Texture2D)。通过 property 参数指定属性名，默认 texture。TextureButton 支持 texture_normal/pressed/hover/disabled/focused；TextureProgressBar 支持 texture_under/over/progress", {"type": "object", "properties": {}, "required": []}),
        ("add_circle_collision", "为节点添加圆形碰撞体 (CollisionShape2D + CircleShape2D)", {"type": "object", "properties": {}, "required": []}),
        ("add_rectangle_collision", "为节点添加矩形碰撞体 (CollisionShape2D + RectangleShape2D)", {"type": "object", "properties": {}, "required": []}),
        ("find_nodes_by_name", "按名称子串搜索节点（大小写敏感，包含匹配）", {"type": "object", "properties": {}, "required": []}),
        ("find_nodes_by_type", "按节点类型精确搜索", {"type": "object", "properties": {}, "required": []}),
        ("find_nodes_by_group", "按组名搜索节点", {"type": "object", "properties": {}, "required": []}),
        ("find_nodes_by_script", "按脚本路径搜索节点", {"type": "object", "properties": {}, "required": []}),
        ("call_method", "调用节点方法", {"type": "object", "properties": {}, "required": []}),
        ("get_variable", "读取节点脚本变量（编辑器模式下仅支持 @export 变量）", {"type": "object", "properties": {}, "required": []}),
        ("set_variable", "写入节点脚本变量（编辑器模式下仅支持 @export 变量）", {"type": "object", "properties": {}, "required": []}),
        ("get_project_setting", "读取项目设置", {"type": "object", "properties": {"property": {"type": "string"}}, "required": ["property"]}),
        ("set_project_setting", "写入项目设置", {"type": "object", "properties": {}, "required": []}),
        ("set_main_scene", "设置主场景", {"type": "object", "properties": {"scene_path": {"type": "string"}}, "required": ["scene_path"]}),
        ("create_scene", "创建新场景文件 (可选 root_type/root_name)", {"type": "object", "properties": {}, "required": []}),
        ("delete_scene", "删除场景文件", {"type": "object", "properties": {"path": {"type": "string"}}, "required": ["path"]}),
        ("rename_scene", "重命名/移动场景文件", {"type": "object", "properties": {}, "required": []}),
        ("branch_to_scene", "将节点分支转为场景文件", {"type": "object", "properties": {}, "required": []}),
        ("scene_to_branch", "将实例化场景转为本地分支", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("instantiate_scene", "实例化子场景", {"type": "object", "properties": {}, "required": []}),
        ("open_scene", "在编辑器中打开场景文件", {"type": "object", "properties": {}, "required": []}),
        ("close_scene", "关闭当前编辑场景标签", {"type": "object", "properties": {}, "required": []}),
        ("save_scene", "保存当前编辑场景到原文件", {"type": "object", "properties": {}, "required": []}),
        ("save_scene_as", "将当前编辑场景另存为新路径", {"type": "object", "properties": {}, "required": []}),
        ("save_all_scenes", "保存所有已打开的场景", {"type": "object", "properties": {}, "required": []}),
        ("reload_scene", "从磁盘重新加载指定场景", {"type": "object", "properties": {"scene_path": {"type": "string"}}, "required": ["scene_path"]}),
        ("get_open_scenes", "列出所有已打开的场景文件路径", {"type": "object", "properties": {}, "required": []}),
        ("get_open_scene_roots", "列出所有已打开场景的根节点信息", {"type": "object", "properties": {}, "required": []}),
        ("mark_scene_unsaved", "标记当前编辑场景为未保存", {"type": "object", "properties": {}, "required": []}),
        ("create_gdscript", "创建 GDScript 文件", {"type": "object", "properties": {}, "required": []}),
        ("read_gdscript", "读取 GDScript 文件源码", {"type": "object", "properties": {}, "required": []}),
        ("edit_gdscript", "修改 GDScript 文件源码", {"type": "object", "properties": {}, "required": []}),
        ("validate_gdscript", "验证 GDScript 语法 (通过 Godot LSP)", {"type": "object", "properties": {"path": {"type": "string"}}, "required": ["path"]}),
        ("list_gdscripts", "列出项目所有 GDScript 文件", {"type": "object", "properties": {}, "required": []}),
        ("create_csharp_script", "创建 C# 脚本文件 (需先执行 csharp_create_solution)", {"type": "object", "properties": {}, "required": []}),
        ("read_csharp_script", "读取 C# 脚本文件源码", {"type": "object", "properties": {"path": {"type": "string"}}, "required": ["path"]}),
        ("edit_csharp_script", "修改 C# 脚本文件源码 (需执行 csharp_build 编译)", {"type": "object", "properties": {}, "required": []}),
        ("list_csharp_scripts", "列出项目所有 C# 脚本文件", {"type": "object", "properties": {}, "required": []}),
        ("csharp_build", "编译 C# 项目 (调用 dotnet build)", {"type": "object", "properties": {"configuration": {"type": "string"}}, "required": []}),
        ("csharp_create_solution", "创建 C# Solution 文件", {"type": "object", "properties": {}, "required": []}),
        ("find_in_file", "在单个文件中搜索文本或正则", {"type": "object", "properties": {}, "required": []}),
        ("search_project", "在项目中全文搜索 (递归)", {"type": "object", "properties": {}, "required": []}),
        ("godot_editor_open", "启动 Godot 编辑器并打开项目", {"type": "object", "properties": {}, "required": []}),
        ("godot_editor_close", "关闭 Godot 编辑器进程", {"type": "object", "properties": {}, "required": []}),
        ("godot_editor_restart", "重启 Godot 编辑器", {"type": "object", "properties": {}, "required": []}),
        ("undo", "撤销最近一次操作", {"type": "object", "properties": {}, "required": []}),
        ("redo", "重做最近一次操作", {"type": "object", "properties": {}, "required": []}),
        ("get_node_position_3d", "获取 Node3D 节点的 Vector3 位置", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_position_3d", "设置 Node3D 节点的 Vector3 位置", {"type": "object", "properties": {}, "required": []}),
        ("get_node_rotation_3d", "获取 Node3D 节点的 Euler 角旋转（度数）", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_rotation_3d", "设置 Node3D 节点的 Euler 角旋转（度数）", {"type": "object", "properties": {}, "required": []}),
        ("get_node_scale_3d", "获取 Node3D 节点的 Vector3 缩放", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("set_node_scale_3d", "设置 Node3D 节点的 Vector3 缩放", {"type": "object", "properties": {}, "required": []}),
        ("set_node_transform_2d", "一次性设置 2D 节点的 position + rotation + scale（单个 undo action）", {"type": "object", "properties": {}, "required": []}),
        ("set_node_transform_3d", "一次性设置 3D 节点的 position + rotation + scale（单个 undo action）", {"type": "object", "properties": {}, "required": []}),
        ("get_node_info", "返回节点的完整信息：类型、脚本路径、可见性、组、子节点数、owner、unique_name、是否为实例化场景", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("get_script_variables", "列出节点脚本上的所有 @export 变量名、类型和当前值", {"type": "object", "properties": {"node_path": {"type": "string"}}, "required": ["node_path"]}),
        ("is_scene_dirty", "查询当前场景是否有未保存的变更", {"type": "object", "properties": {}, "required": []}),
        ("find_and_replace", "项目级查找替换（脚本/tscn 文件中的文本）", {"type": "object", "properties": {}, "required": []}),
        ("play_current_scene", "播放当前编辑器中打开的场景", {"type": "object", "properties": {}, "required": []}),
        ("play_main_scene", "播放项目主场景", {"type": "object", "properties": {}, "required": []}),
        ("stop_scene", "停止正在运行的场景", {"type": "object", "properties": {}, "required": []}),
        ("is_scene_playing", "查询场景是否正在播放", {"type": "object", "properties": {}, "required": []}),
        ("refresh_filesystem", "刷新编辑器文件系统扫描", {"type": "object", "properties": {}, "required": []}),
        ("get_editor_info", "获取编辑器信息：引擎版本、编辑器路径、缩放比例、语言", {"type": "object", "properties": {}, "required": []}),
        ("list_autoloads", "列出项目所有 Autoload 单例", {"type": "object", "properties": {}, "required": []}),
        ("add_autoload", "添加 Autoload 单例", {"type": "object", "properties": {}, "required": []}),
        ("remove_autoload", "移除 Autoload 单例", {"type": "object", "properties": {"name": {"type": "string"}}, "required": ["name"]}),
        ("list_scenes", "列出项目中所有场景文件 (.tscn/.scn)", {"type": "object", "properties": {}, "required": []}),
        ("get_display_settings", "获取显示/窗口相关设置（分辨率、拉伸、全屏模式等）", {"type": "object", "properties": {}, "required": []}),
        ("set_display_settings", "设置显示/窗口相关设置", {"type": "object", "properties": {}, "required": []}),
        ("get_project_info", "获取项目基本信息（名称、描述、版本、图标、主场景等）", {"type": "object", "properties": {}, "required": []}),
        ("set_project_info", "设置项目基本信息", {"type": "object", "properties": {}, "required": []}),
        ("get_physics_settings", "获取物理引擎设置（2D/3D 重力、阻尼、帧率等）", {"type": "object", "properties": {}, "required": []}),
        ("set_physics_settings", "设置物理引擎参数", {"type": "object", "properties": {}, "required": []}),
        ("get_rendering_settings", "获取渲染相关设置（渲染器、抗锯齿、像素对齐等）", {"type": "object", "properties": {}, "required": []}),
        ("set_rendering_settings", "设置渲染参数", {"type": "object", "properties": {}, "required": []}),
        ("get_layer_names", "获取物理/导航/渲染层名称（按分类）", {"type": "object", "properties": {}, "required": []}),
        ("set_layer_names", "设置物理/导航/渲染层名称", {"type": "object", "properties": {}, "required": []}),
        ("list_plugins", "列出 res://addons/ 中所有插件及其启用状态", {"type": "object", "properties": {}, "required": []}),
        ("set_plugin_enabled", "启用或禁用编辑器插件", {"type": "object", "properties": {}, "required": []}),
        ("list_input_actions", "列出输入动作及其绑定的事件", {"type": "object", "properties": {}, "required": []}),
        ("add_input_action", "添加新的输入动作", {"type": "object", "properties": {}, "required": []}),
        ("set_input_action_events", "设置输入动作绑定的事件（替换/追加/清空）", {"type": "object", "properties": {}, "required": []}),
        ("remove_input_action", "移除输入动作", {"type": "object", "properties": {"name": {"type": "string"}}, "required": ["name"]}),
    ]

    def __init__(self) -> None:
        self._tools: dict[str, ToolInfo] = {}
        for name, desc, schema in self._TOOLS:
            self._tools[name] = ToolInfo(
                name=name,
                description=desc,
                input_schema=schema,
                enabled=True,
            )

    @property
    def total(self) -> int:
        return len(self._tools)

    def get_all_tools(self) -> list[ToolInfo]:
        return list(self._tools.values())

    def get_enabled_tools(self) -> list[ToolInfo]:
        return [t for t in self._tools.values() if t.enabled]

    def has_tool(self, name: str) -> bool:
        return name in self._tools

    def is_tool_enabled(self, name: str) -> bool:
        t = self._tools.get(name)
        return t.enabled if t is not None else False

    def set_tool_enabled(self, name: str, enabled: bool) -> bool:
        if name not in self._tools:
            return False
        self._tools[name].enabled = enabled
        return True

    def register_tool(self, name: str, description: str, schema: dict) -> None:
        self._tools[name] = ToolInfo(
            name=name,
            description=description,
            input_schema=schema,
            enabled=True,
        )

    def update_from_notification(self, update: ToolListUpdate) -> None:
        for ts in update.tools:
            if ts.name in self._tools:
                self._tools[ts.name].enabled = ts.enabled

    def tool_count(self) -> tuple[int, int]:
        enabled = sum(1 for t in self._tools.values() if t.enabled)
        return (enabled, len(self._tools))

