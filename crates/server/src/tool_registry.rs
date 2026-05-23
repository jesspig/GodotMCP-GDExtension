use dashmap::DashMap;
use serde_json::json;
use std::sync::Arc;

use godot_mcp_core::tool_manifest::{ToolInfo, ToolListUpdate};

#[derive(Clone)]
pub struct ToolRegistry {
    tools: Arc<DashMap<String, ToolEntry>>,
}

struct ToolEntry {
    info: ToolInfo,
    enabled: bool,
}

impl ToolRegistry {
    pub fn new() -> Self {
        let registry = Self {
            tools: Arc::new(DashMap::new()),
        };
        registry.register_defaults();
        registry
    }

    fn register_defaults(&self) {
        let schema = |props: &str, required: &[&str]| -> serde_json::Value {
            let req: Vec<&str> = required.to_vec();
            json!({"type":"object","properties":serde_json::from_str::<serde_json::Value>(props).unwrap_or(json!({})),"required":req})
        };
        let empty = json!({"type":"object","properties":{},"required":[]});

        let defaults: Vec<(&str, &str, serde_json::Value)> = vec![
            // ── Meta (4) ───────────────────────────────────────────────
            (
                "ping",
                "检测与 Godot 编辑器的连接状态（编辑器刚启动时可能需要等待几秒后再调用）",
                empty.clone(),
            ),
            ("get_engine_version", "获取 Godot 引擎版本号", empty.clone()),
            (
                "get_plugin_version",
                "获取 Godot MCP 插件版本号",
                empty.clone(),
            ),
            (
                "get_server_version",
                "获取 godot-mcp-server 版本号",
                empty.clone(),
            ),
            // ── Node: Read (4) ─────────────────────────────────────────
            (
                "get_scene_tree",
                "获取当前场景节点树",
                schema(r#"{"max_depth":{"type":"integer"}}"#, &[]),
            ),
            (
                "get_node_path",
                "获取节点路径",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "get_property_list",
                "获取节点所有属性列表",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "get_property",
                "获取节点指定属性值",
                schema(
                    r#"{"node_path":{"type":"string"},"property":{"type":"string"}}"#,
                    &["node_path", "property"],
                ),
            ),
            // ── Node: Write (13) ───────────────────────────────────────
            (
                "create_node",
                "创建新节点",
                schema(
                    r#"{"parent_path":{"type":"string"},"node_type":{"type":"string"},"name":{"type":"string"}}"#,
                    &["parent_path", "node_type", "name"],
                ),
            ),
            (
                "delete_node",
                "删除节点",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "rename_node",
                "重命名节点",
                schema(
                    r#"{"node_path":{"type":"string"},"new_name":{"type":"string"}}"#,
                    &["node_path", "new_name"],
                ),
            ),
            (
                "set_property",
                "修改节点属性值 (通用兜底)",
                schema(
                    r#"{"node_path":{"type":"string"},"property":{"type":"string"},"value":{}}"#,
                    &["node_path", "property", "value"],
                ),
            ),
            (
                "duplicate_node",
                "复制节点",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "move_node",
                "移动节点到新父节点",
                schema(
                    r#"{"node_path":{"type":"string"},"new_parent_path":{"type":"string"}}"#,
                    &["node_path", "new_parent_path"],
                ),
            ),
            (
                "attach_script",
                "为节点挂载脚本",
                schema(
                    r#"{"node_path":{"type":"string"},"script_path":{"type":"string"}}"#,
                    &["node_path", "script_path"],
                ),
            ),
            (
                "detach_script",
                "卸载节点脚本",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "reset_parent",
                "重设父节点 (reparent)",
                schema(
                    r#"{"node_path":{"type":"string"},"new_parent_path":{"type":"string"}}"#,
                    &["node_path", "new_parent_path"],
                ),
            ),
            (
                "set_as_root",
                "设置节点为场景根",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "batch_set_property",
                "批量修改多个节点同名属性",
                schema(
                    r#"{"node_paths":{"type":"array"},"property":{"type":"string"},"value":{}}"#,
                    &["node_paths", "property", "value"],
                ),
            ),
            (
                "add_node_to_group",
                "添加节点到组",
                schema(
                    r#"{"node_path":{"type":"string"},"group":{"type":"string"}}"#,
                    &["node_path", "group"],
                ),
            ),
            (
                "remove_node_from_group",
                "从组中移除节点",
                schema(
                    r#"{"node_path":{"type":"string"},"group":{"type":"string"}}"#,
                    &["node_path", "group"],
                ),
            ),
            // ── Property: get/set (19) ─────────────────────────────────
            (
                "get_node_position",
                "获取节点位置",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_position",
                "设置节点位置",
                schema(
                    r#"{"node_path":{"type":"string"},"x":{"type":"number"},"y":{"type":"number"}}"#,
                    &["node_path", "x", "y"],
                ),
            ),
            (
                "get_node_rotation",
                "获取节点旋转角度 (度)",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_rotation",
                "设置节点旋转角度 (度)",
                schema(
                    r#"{"node_path":{"type":"string"},"degrees":{"type":"number"}}"#,
                    &["node_path", "degrees"],
                ),
            ),
            (
                "get_node_scale",
                "获取节点缩放",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_scale",
                "设置节点缩放",
                schema(
                    r#"{"node_path":{"type":"string"},"x":{"type":"number"},"y":{"type":"number"}}"#,
                    &["node_path", "x", "y"],
                ),
            ),
            (
                "get_node_visible",
                "获取节点可见性",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_visible",
                "设置节点可见性",
                schema(
                    r#"{"node_path":{"type":"string"},"visible":{"type":"boolean"}}"#,
                    &["node_path", "visible"],
                ),
            ),
            (
                "get_node_modulate",
                "获取节点调制颜色",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_modulate",
                "设置节点调制颜色",
                schema(
                    r#"{"node_path":{"type":"string"},"r":{"type":"number"},"g":{"type":"number"},"b":{"type":"number"},"a":{"type":"number"}}"#,
                    &["node_path"],
                ),
            ),
            (
                "get_node_z_index",
                "获取节点 Z 轴索引",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_z_index",
                "设置节点 Z 轴索引",
                schema(
                    r#"{"node_path":{"type":"string"},"z_index":{"type":"integer"}}"#,
                    &["node_path", "z_index"],
                ),
            ),
            (
                "get_node_text",
                "获取节点文本 (Label 等)",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_text",
                "设置节点文本 (Label 等)",
                schema(
                    r#"{"node_path":{"type":"string"},"text":{"type":"string"}}"#,
                    &["node_path", "text"],
                ),
            ),
            (
                "get_node_collision_layer",
                "获取碰撞层",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_collision_layer",
                "设置碰撞层",
                schema(
                    r#"{"node_path":{"type":"string"},"layer":{"type":"integer"}}"#,
                    &["node_path", "layer"],
                ),
            ),
            (
                "get_node_collision_mask",
                "获取碰撞掩码",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "set_node_collision_mask",
                "设置碰撞掩码",
                schema(
                    r#"{"node_path":{"type":"string"},"mask":{"type":"integer"}}"#,
                    &["node_path", "mask"],
                ),
            ),
            (
                "set_node_unique_name",
                "设置节点唯一名称 (%前缀)",
                schema(
                    r#"{"node_path":{"type":"string"},"unique":{"type":"boolean"}}"#,
                    &["node_path"],
                ),
            ),
            (
                "get_node_texture",
                "获取节点纹理属性 (Texture2D)。通过 property 参数指定属性名，默认 texture。TextureButton 支持 texture_normal/pressed/hover/disabled/focused；TextureProgressBar 支持 texture_under/over/progress",
                schema(
                    r#"{"node_path":{"type":"string"},"property":{"type":"string","description":"纹理属性名，默认 texture"}}"#,
                    &["node_path"],
                ),
            ),
            (
                "set_node_texture",
                "设置节点纹理属性 (Texture2D)。通过 property 参数指定属性名，默认 texture。TextureButton 支持 texture_normal/pressed/hover/disabled/focused；TextureProgressBar 支持 texture_under/over/progress",
                schema(
                    r#"{"node_path":{"type":"string"},"texture_path":{"type":"string","description":"纹理资源路径，如 res://icon.svg"},"property":{"type":"string","description":"纹理属性名，默认 texture"}}"#,
                    &["node_path", "texture_path"],
                ),
            ),
            // ── Collision (2) ──────────────────────────────────────────
            (
                "add_circle_collision",
                "为节点添加圆形碰撞体 (CollisionShape2D + CircleShape2D)",
                schema(
                    r#"{"node_path":{"type":"string"},"radius":{"type":"number"}}"#,
                    &["node_path"],
                ),
            ),
            (
                "add_rectangle_collision",
                "为节点添加矩形碰撞体 (CollisionShape2D + RectangleShape2D)",
                schema(
                    r#"{"node_path":{"type":"string"},"width":{"type":"number"},"height":{"type":"number"}}"#,
                    &["node_path"],
                ),
            ),
            // ── Find (4) ───────────────────────────────────────────────
            (
                "find_nodes_by_name",
                "按名称子串搜索节点（大小写敏感，包含匹配）",
                schema(
                    r#"{"pattern":{"type":"string"},"max_results":{"type":"integer"}}"#,
                    &["pattern"],
                ),
            ),
            (
                "find_nodes_by_type",
                "按节点类型精确搜索",
                schema(
                    r#"{"node_class":{"type":"string"},"max_results":{"type":"integer"}}"#,
                    &["node_class"],
                ),
            ),
            (
                "find_nodes_by_group",
                "按组名搜索节点",
                schema(
                    r#"{"group":{"type":"string"},"max_results":{"type":"integer"}}"#,
                    &["group"],
                ),
            ),
            (
                "find_nodes_by_script",
                "按脚本路径搜索节点",
                schema(
                    r#"{"script_path":{"type":"string"},"max_results":{"type":"integer"}}"#,
                    &["script_path"],
                ),
            ),
            // ── Script helpers (3) ─────────────────────────────────────
            (
                "call_method",
                "调用节点方法",
                schema(
                    r#"{"node_path":{"type":"string"},"method":{"type":"string"},"args":{"type":"array"}}"#,
                    &["node_path", "method"],
                ),
            ),
            (
                "get_variable",
                "读取节点脚本变量（编辑器模式下仅支持 @export 变量）",
                schema(
                    r#"{"node_path":{"type":"string"},"variable":{"type":"string"}}"#,
                    &["node_path", "variable"],
                ),
            ),
            (
                "set_variable",
                "写入节点脚本变量（编辑器模式下仅支持 @export 变量）",
                schema(
                    r#"{"node_path":{"type":"string"},"variable":{"type":"string"},"value":{}}"#,
                    &["node_path", "variable", "value"],
                ),
            ),
            // ── Project settings (3) ───────────────────────────────────
            (
                "get_project_setting",
                "读取项目设置",
                schema(r#"{"property":{"type":"string"}}"#, &["property"]),
            ),
            (
                "set_project_setting",
                "写入项目设置",
                schema(
                    r#"{"property":{"type":"string"},"value":{}}"#,
                    &["property", "value"],
                ),
            ),
            (
                "set_main_scene",
                "设置主场景",
                schema(r#"{"scene_path":{"type":"string"}}"#, &["scene_path"]),
            ),
            // ── Scene: file (6) ────────────────────────────────────────
            (
                "create_scene",
                "创建新场景文件 (可选 root_type/root_name)",
                schema(
                    r#"{"path":{"type":"string"},"root_type":{"type":"string"},"root_name":{"type":"string"}}"#,
                    &["path"],
                ),
            ),
            (
                "delete_scene",
                "删除场景文件",
                schema(r#"{"path":{"type":"string"}}"#, &["path"]),
            ),
            (
                "rename_scene",
                "重命名/移动场景文件",
                schema(
                    r#"{"source_path":{"type":"string"},"dest_path":{"type":"string"}}"#,
                    &["source_path", "dest_path"],
                ),
            ),
            (
                "branch_to_scene",
                "将节点分支转为场景文件",
                schema(
                    r#"{"node_path":{"type":"string"},"scene_path":{"type":"string"}}"#,
                    &["node_path", "scene_path"],
                ),
            ),
            (
                "scene_to_branch",
                "将实例化场景转为本地分支",
                schema(r#"{"node_path":{"type":"string"}}"#, &["node_path"]),
            ),
            (
                "instantiate_scene",
                "实例化子场景",
                schema(
                    r#"{"scene_path":{"type":"string"},"parent_path":{"type":"string"}}"#,
                    &["scene_path", "parent_path"],
                ),
            ),
            // ── Scene: editor tabs (9) ──────────────────────────────────
            (
                "open_scene",
                "在编辑器中打开场景文件",
                schema(
                    r#"{"scene_path":{"type":"string"},"set_inherited":{"type":"boolean"}}"#,
                    &["scene_path"],
                ),
            ),
            ("close_scene", "关闭当前编辑场景标签", empty.clone()),
            ("save_scene", "保存当前编辑场景到原文件", empty.clone()),
            (
                "save_scene_as",
                "将当前编辑场景另存为新路径",
                schema(
                    r#"{"scene_path":{"type":"string"},"with_preview":{"type":"boolean"}}"#,
                    &["scene_path"],
                ),
            ),
            ("save_all_scenes", "保存所有已打开的场景", empty.clone()),
            (
                "reload_scene",
                "从磁盘重新加载指定场景",
                schema(r#"{"scene_path":{"type":"string"}}"#, &["scene_path"]),
            ),
            (
                "get_open_scenes",
                "列出所有已打开的场景文件路径",
                empty.clone(),
            ),
            (
                "get_open_scene_roots",
                "列出所有已打开场景的根节点信息",
                empty.clone(),
            ),
            (
                "mark_scene_unsaved",
                "标记当前编辑场景为未保存",
                empty.clone(),
            ),
            // ── GDScript (5) ────────────────────────────────────────────
            (
                "create_gdscript",
                "创建 GDScript 文件",
                schema(
                    r#"{"path":{"type":"string"},"base_class":{"type":"string"},"class_name":{"type":"string"},"template":{"type":"string"},"overwrite":{"type":"boolean"}}"#,
                    &["path", "base_class"],
                ),
            ),
            (
                "read_gdscript",
                "读取 GDScript 文件源码",
                schema(
                    r#"{"path":{"type":"string"},"from_editor":{"type":"boolean"}}"#,
                    &["path"],
                ),
            ),
            (
                "edit_gdscript",
                "修改 GDScript 文件源码",
                schema(
                    r#"{"path":{"type":"string"},"source":{"type":"string"}}"#,
                    &["path", "source"],
                ),
            ),
            (
                "validate_gdscript",
                "验证 GDScript 语法 (通过 Godot LSP)",
                schema(r#"{"path":{"type":"string"}}"#, &["path"]),
            ),
            (
                "list_gdscripts",
                "列出项目所有 GDScript 文件",
                schema(
                    r#"{"root":{"type":"string"},"include_addons":{"type":"boolean"},"max_results":{"type":"integer"}}"#,
                    &[],
                ),
            ),
            // ── C# (6) ──────────────────────────────────────────────────
            (
                "create_csharp_script",
                "创建 C# 脚本文件 (需先执行 csharp_create_solution)",
                schema(
                    r#"{"path":{"type":"string"},"base_class":{"type":"string"},"overwrite":{"type":"boolean"}}"#,
                    &["path", "base_class"],
                ),
            ),
            (
                "read_csharp_script",
                "读取 C# 脚本文件源码",
                schema(r#"{"path":{"type":"string"}}"#, &["path"]),
            ),
            (
                "edit_csharp_script",
                "修改 C# 脚本文件源码 (需执行 csharp_build 编译)",
                schema(
                    r#"{"path":{"type":"string"},"source":{"type":"string"}}"#,
                    &["path", "source"],
                ),
            ),
            (
                "list_csharp_scripts",
                "列出项目所有 C# 脚本文件",
                schema(
                    r#"{"root":{"type":"string"},"include_addons":{"type":"boolean"},"max_results":{"type":"integer"}}"#,
                    &[],
                ),
            ),
            (
                "csharp_build",
                "编译 C# 项目 (调用 dotnet build)",
                schema(r#"{"configuration":{"type":"string"}}"#, &[]),
            ),
            (
                "csharp_create_solution",
                "创建 C# Solution 文件",
                schema(
                    r#"{"enable_nativeaot":{"type":"boolean","description":"可选，启用 NativeAOT 支持"}}"#,
                    &[],
                ),
            ),
            // ── Search (2) ──────────────────────────────────────────────
            (
                "find_in_file",
                "在单个文件中搜索文本或正则",
                schema(
                    r#"{"path":{"type":"string"},"pattern":{"type":"string"},"literal":{"type":"boolean"},"case_sensitive":{"type":"boolean"},"max_matches":{"type":"integer"}}"#,
                    &["path", "pattern"],
                ),
            ),
            (
                "search_project",
                "在项目中全文搜索 (递归)",
                schema(
                    r#"{"pattern":{"type":"string"},"literal":{"type":"boolean"},"extensions":{"type":"array"},"root":{"type":"string"},"case_sensitive":{"type":"boolean"},"max_matches":{"type":"integer"},"max_files":{"type":"integer"}}"#,
                    &["pattern"],
                ),
            ),
            // ── Editor control (3, server-side) ─────────────────────────
            (
                "godot_editor_open",
                "启动 Godot 编辑器并打开项目",
                schema(
                    r#"{"project_path":{"type":"string","description":"Godot 项目目录路径，默认为 godot/"}}"#,
                    &[],
                ),
            ),
            ("godot_editor_close", "关闭 Godot 编辑器进程", empty.clone()),
            (
                "godot_editor_restart",
                "重启 Godot 编辑器",
                schema(
                    r#"{"project_path":{"type":"string","description":"Godot 项目目录路径，默认为 godot/"}}"#,
                    &[],
                ),
            ),
        ];

        for (name, desc, schema) in defaults {
            self.tools.insert(
                name.to_string(),
                ToolEntry {
                    info: ToolInfo {
                        name: name.to_string(),
                        description: desc.to_string(),
                        input_schema: schema,
                        enabled: true,
                    },
                    enabled: true,
                },
            );
        }
    }

    #[allow(dead_code)]
    pub fn register_tool(&self, name: &str, description: &str, schema: serde_json::Value) {
        self.tools.insert(
            name.to_string(),
            ToolEntry {
                info: ToolInfo {
                    name: name.to_string(),
                    description: description.to_string(),
                    input_schema: schema,
                    enabled: true,
                },
                enabled: true,
            },
        );
    }

    #[allow(dead_code)]
    pub fn set_tool_enabled(&self, tool_name: &str, enabled: bool) -> bool {
        if let Some(mut entry) = self.tools.get_mut(tool_name) {
            entry.enabled = enabled;
            entry.info.enabled = enabled;
            true
        } else {
            false
        }
    }

    pub fn is_tool_enabled(&self, tool_name: &str) -> bool {
        self.tools
            .get(tool_name)
            .map(|e| e.enabled)
            .unwrap_or(false)
    }

    pub fn has_tool(&self, tool_name: &str) -> bool {
        self.tools.contains_key(tool_name)
    }

    pub fn get_enabled_tools(&self) -> Vec<ToolInfo> {
        self.tools
            .iter()
            .filter(|e| e.enabled)
            .map(|e| e.info.clone())
            .collect()
    }

    #[allow(dead_code)]
    pub fn get_all_tools(&self) -> Vec<ToolInfo> {
        self.tools.iter().map(|e| e.info.clone()).collect()
    }

    #[allow(dead_code)]
    pub fn update_from_notification(&self, update: &ToolListUpdate) {
        for tool_state in &update.tools {
            self.set_tool_enabled(&tool_state.name, tool_state.enabled);
        }
    }

    #[allow(dead_code)]
    pub fn tool_count(&self) -> (usize, usize) {
        let total = self.tools.len();
        let enabled = self.tools.iter().filter(|e| e.enabled).count();
        (enabled, total)
    }
}

impl Default for ToolRegistry {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use godot_mcp_core::tool_manifest::ToolState;

    #[test]
    fn new_registry_has_defaults() {
        let registry = ToolRegistry::new();
        let (enabled, total) = registry.tool_count();
        assert_eq!(total, 85);
        assert_eq!(enabled, 85);
    }

    #[test]
    fn all_defaults_enabled() {
        let registry = ToolRegistry::new();
        assert!(registry.is_tool_enabled("ping"));
        assert!(registry.is_tool_enabled("get_engine_version"));
        assert!(registry.is_tool_enabled("get_plugin_version"));
        assert!(registry.is_tool_enabled("get_server_version"));
    }

    #[test]
    fn disable_tool() {
        let registry = ToolRegistry::new();
        assert!(registry.set_tool_enabled("ping", false));
        assert!(!registry.is_tool_enabled("ping"));
        let (enabled, _) = registry.tool_count();
        assert_eq!(enabled, 84);
    }

    #[test]
    fn enable_tool() {
        let registry = ToolRegistry::new();
        registry.set_tool_enabled("ping", false);
        assert!(registry.set_tool_enabled("ping", true));
        assert!(registry.is_tool_enabled("ping"));
    }

    #[test]
    fn set_nonexistent_tool() {
        let registry = ToolRegistry::new();
        assert!(!registry.set_tool_enabled("nonexistent", true));
    }

    #[test]
    fn is_tool_enabled_unknown() {
        let registry = ToolRegistry::new();
        assert!(!registry.is_tool_enabled("unknown_tool"));
    }

    #[test]
    fn register_custom_tool() {
        let registry = ToolRegistry::new();
        registry.register_tool("custom_tool", "A custom tool", json!({}));
        assert!(registry.is_tool_enabled("custom_tool"));
        let (_, total) = registry.tool_count();
        assert_eq!(total, 86);
    }

    #[test]
    fn get_enabled_tools_respects_state() {
        let registry = ToolRegistry::new();
        registry.set_tool_enabled("ping", false);
        registry.set_tool_enabled("get_engine_version", false);
        let enabled = registry.get_enabled_tools();
        assert_eq!(enabled.len(), 83);
        let names: Vec<&str> = enabled.iter().map(|t| t.name.as_str()).collect();
        assert!(names.contains(&"get_plugin_version"));
        assert!(names.contains(&"get_server_version"));
    }

    #[test]
    fn update_from_notification() {
        let registry = ToolRegistry::new();
        let update = ToolListUpdate {
            tools: vec![
                ToolState {
                    name: "ping".into(),
                    enabled: false,
                },
                ToolState {
                    name: "get_engine_version".into(),
                    enabled: false,
                },
            ],
        };
        registry.update_from_notification(&update);
        assert!(!registry.is_tool_enabled("ping"));
        assert!(!registry.is_tool_enabled("get_engine_version"));
        assert!(registry.is_tool_enabled("get_plugin_version"));
        let (enabled, _) = registry.tool_count();
        assert_eq!(enabled, 83);
    }

    #[test]
    fn update_notification_unknown_tool_ignored() {
        let registry = ToolRegistry::new();
        let update = ToolListUpdate {
            tools: vec![ToolState {
                name: "nonexistent".into(),
                enabled: false,
            }],
        };
        registry.update_from_notification(&update);
        let (_, total) = registry.tool_count();
        assert_eq!(total, 85);
    }
}
