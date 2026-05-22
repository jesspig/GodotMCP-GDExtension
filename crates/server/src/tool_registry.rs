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
            // Meta tools
            ("ping", "检测与 Godot 编辑器的连接状态", empty.clone()),
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
            // Scene Management: Read
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
            // Scene Management: Node write
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
                "修改节点属性值",
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
            // Scene Management: Script + search
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
                "find_nodes",
                "按条件搜索节点",
                schema(
                    r#"{"query":{"type":"string"},"search_method":{"type":"string"}}"#,
                    &["query"],
                ),
            ),
            // Scene Management: Scene file
            (
                "create_scene",
                "创建新空场景文件",
                schema(r#"{"path":{"type":"string"}}"#, &["path"]),
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
            // Scene Management: Advanced
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
            // Scene Management: Editor scene tabs (open/close/save/reload)
            (
                "open_scene",
                "在编辑器中打开 .tscn 场景文件 (set_inherited 可选)",
                schema(
                    r#"{"scene_path":{"type":"string"},"set_inherited":{"type":"boolean"}}"#,
                    &["scene_path"],
                ),
            ),
            ("close_scene", "关闭当前编辑场景标签", empty.clone()),
            ("save_scene", "保存当前编辑场景到原文件", empty.clone()),
            (
                "save_scene_as",
                "将当前编辑场景另存为新路径 (with_preview 可选, 默认 true)",
                schema(
                    r#"{"scene_path":{"type":"string"},"with_preview":{"type":"boolean"}}"#,
                    &["scene_path"],
                ),
            ),
            ("save_all_scenes", "保存所有已打开的场景", empty.clone()),
            (
                "reload_scene",
                "从磁盘重新加载指定场景 (外部修改后同步)",
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
                "标记当前编辑场景为未保存 (标签出现 * 号)",
                empty.clone(),
            ),
            // GDScript tools
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
            (
                "eval_gdscript_expression",
                "评估 GDScript 表达式 (const_only 默认 true)",
                schema(
                    r#"{"expression":{"type":"string"},"node_path":{"type":"string"},"const_only":{"type":"boolean"}}"#,
                    &["expression"],
                ),
            ),
            // CSharp tools
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
                "创建 C# Solution 文件 (调用 godot --build-solutions)",
                empty.clone(),
            ),
            // Search tools
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
        assert_eq!(total, 49);
        assert_eq!(enabled, 49);
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
        assert_eq!(enabled, 48);
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
        assert_eq!(total, 50);
    }

    #[test]
    fn get_enabled_tools_respects_state() {
        let registry = ToolRegistry::new();
        registry.set_tool_enabled("ping", false);
        registry.set_tool_enabled("get_engine_version", false);
        let enabled = registry.get_enabled_tools();
        assert_eq!(enabled.len(), 47);
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
        assert_eq!(enabled, 47);
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
        assert_eq!(total, 49);
    }
}
