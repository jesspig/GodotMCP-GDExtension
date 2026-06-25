# 插件管理工具

> 2 个工具，管理 `res://addons/` 中的编辑器插件。位于 `extensions/src/built_in/tools/editor_tools/plugin/`，通过 X-macro 注册。

## 工具列表

| 工具 | 描述 |
|------|------|
| `list_plugins` | 列出 `res://addons/` 中所有插件及其启用状态 |
| `set_plugin_enabled` | 启用或禁用编辑器插件 |

## `list_plugins`

扫描 `res://addons/` 目录下每个子目录的 `plugin.cfg` 文件。

**返回**：插件列表，每个插件包含 name、version、author、description 和 enabled 状态。

```json
{
  "plugins": [
    {
      "name": "Godot MCP",
      "version": "0.2.2-dev3",
      "author": "",
      "description": "Model Context Protocol bridge for Godot Engine.",
      "enabled": true
    }
  ]
}
```

## `set_plugin_enabled`

**参数**：

- `plugin_name`：插件名（字符串，必填）
- `enabled`：`true` 启用，`false` 禁用

**实现**：

1. 通过 `args_string(ctx.args, "plugin_name")` 获取插件名
2. 扫描 `res://addons/` 查找匹配目录，构建 `plugin_path`
3. 调用 `EditorInterface::set_plugin_enabled(plugin_path, enabled)`
4. 返回启/禁用结果

## 工作流程

以下序列图展示了调用 `set_plugin_enabled` 时各组件间的交互：

```mermaid
sequenceDiagram
    participant Client as MCP 客户端
    participant HTTP as HTTP Server
    participant Handler as set_plugin_enabled Handler
    participant Editor as EditorInterface

    Client->>HTTP: POST /tools/call (plugin_name, enabled)
    HTTP->>Handler: 分发调用
    Handler->>Editor: EditorInterface::set_plugin_enabled(plugin_path, enabled)
    Editor-->>Handler: 操作结果
    Handler-->>Client: 成功响应
```

## 实现细节

- 插件名用于在 `res://addons/` 下定位插件目录
- 启用/禁用通过 `EditorInterface::set_plugin_enabled(plugin_path, enabled)` API
- 不刷新文件系统，不读取 `plugin.cfg`
