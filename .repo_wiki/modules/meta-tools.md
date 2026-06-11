# 元工具

> 发现工具的系统级工具，始终可见（`is_meta() == true`），用于工具发现、分类浏览、搜索和信息查询。

## 工具列表

6 个元工具，位于 `extensions/src/built_in/tools/meta/`，category 均为 `meta_tools`。

| 工具名 | 文件 | 功能 |
|--------|------|------|
| `get_info` | `get_info.hpp` | 返回连接状态、引擎版本、项目配置、编辑器状态 |
| `get_categories` | `get_categories.hpp` | 返回工具分类树，支持 path 钻取和 max_depth 控制 |
| `get_tools` | `get_tools.hpp` | 按分类路径列出该分类下所有工具（不含子分类） |
| `get_tool_detail` | `get_tool_detail.hpp` | 返回指定工具的完整元数据 |
| `find_tool` | `find_tool.hpp` | 搜索引擎：4 阶段权重（精确>前缀>Token>子串）+ 调用频率排序 |
| `call_tool` | `call_tool.hpp` | 兜底调用任意工具（当 AI 客户端不支持直接 tool call 时使用） |

## 调用流程

```mermaid
sequenceDiagram
    participant Client as AI 客户端
    participant MCP as McpHandler
    participant REG as HandlerRegistry
    participant META as MetaTool

    Client->>MCP: tools/list
    MCP->>REG: get_always_on_tools()
    REG-->>MCP: 仅 is_meta==true 的工具
    MCP-->>Client: [get_info, get_categories, get_tools, find_tool, call_tool, get_tool_detail]

    Client->>MCP: tools/call(get_categories, {path: "node_tools", max_depth: 2})
    MCP->>REG: execute("get_categories", args)
    REG->>META: GetCategoriesTool::execute_impl()
    META->>REG: get_categories()
    REG-->>MCP: 分类树递归结构
    MCP-->>Client: 渐进式分类树

    Client->>MCP: tools/call(find_tool, {query: "scene tree"})
    MCP->>REG: execute("find_tool", args)
    REG->>META: FindTool::execute_impl()
    META->>REG: search_tools(query)
    REG-->>MCP: 按权重+频率排序的工具列表
    MCP-->>Client: 匹配工具列表

    Client->>MCP: tools/call(get_tool_detail, {name: "get_node_position"})
    MCP->>REG: execute("get_tool_detail", args)
    REG->>META: GetToolDetailTool::execute_impl()
    META->>REG: get_tool_schema(name)
    REG-->>MCP: 完整 inputSchema + description
    MCP-->>Client: 单工具完整元数据
```

## 渐进式披露策略

| 阶段 | 操作 | 返回 |
|------|------|------|
| 1 | `tools/list` | 仅 6 个元工具（`is_meta=true`） |
| 2 | `get_categories` | 分类树（首层：meta_tools, node_tools, editor_tools, runtime_tools） |
| 3 | `get_tools(category)` | 指定分类下所有工具 |
| 4 | `find_tool(query)` | 按相关性排序的搜索结果 |
| 5 | `get_tool_detail(name)` | 单工具完整 schema |

## 搜索引擎（`find_tool`）

`HandlerRegistry` 内建搜索引擎，4 阶段权重衰减：

| 阶段 | 匹配方式 | 权重 |
|------|---------|------|
| 1 | 工具名精确匹配 | 1000 |
| 2 | 工具名前缀匹配 | 500 |
| 3 | 工具名/分类/描述 Token 模糊匹配 | 200 |
| 4 | 全文子串匹配 | 50 |

结果按 `权重 × 调用频率` 排序，支持 `get_search_suggestions()` 自动补全。

## 注册

所有元工具通过 X-macro 注册（`register/register_meta.hpp`），`is_meta() → true` 标记始终可见。

## get_info 返回值结构

```json
{
  "connection": { "status": "connected", "transport": "streamable_http" },
  "engine": {
    "version": "4.6",
    "version_string": "4.6.stable.official...",
    "build": "custom_build",
    "godot_version": { "major": 4, "minor": 6, "patch": 0 }
  },
  "plugin": {
    "version": "0.2.0-dev7",
    "builtin_tool_count": 149,
    "custom_tool_count": 0
  },
  "project": {
    "name": "...",
    "path": "res://",
    "data_dir": "C:/Users/.../project_data"
  },
  "editor": {
    "scene_open": true,
    "scene_root_path": "/root/Main",
    "scene_count": 2,
    "open_scenes": ["/root/Main", "/root/Editor"],
    "playing": false,
    "play_mode": "stopped"
  }
}
```

## call_tool 兜底

`call_tool` 将参数中的 `tool` 字段作为工具名重新分派到 `HandlerRegistry::execute()`。用于 AI 客户端不支持 MCP native tool call 时的备用调用：

```json
// 请求
{ "tool": "get_scene_tree", "args": {} }

// 内部
HandlerRegistry::execute("get_scene_tree", {})
```
