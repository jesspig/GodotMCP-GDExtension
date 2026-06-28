# 工具概述

GodotMCP 提供 **164 个内置工具**，组织成四层体系。工具按分类自动发现，可通过 `find_tool` 元工具搜索。

## 四层体系

| 层级 | 数量 | 用途 |
|-------|-------|---------|
| 第 0 层：通用兜底 | 2 | 任何节点的通用属性读取/设置 |
| 第 1 层：元工具 | 9 | 工具发现、内省、配置 |
| 第 2 层：语义工具 | 145 | 针对各编辑器领域的专用工具 |
| 第 3 层：文档查询工具 | 8 | 基于 ClassDB 的 Godot API 参考 |

## 分类树

```
meta_tools/                       # 9 个工具
  get_info, get_tools, get_categories, find_tool, call_tool,
  undo, redo, get_undo_history, execute_workflow

node_tools/
  signal/                         # 4 个工具
  group/                          # 4 个工具
  general/                        # 6 个资源工具
  fallback/                       # 2 个属性工具

editor_tools/
  scene_tree/                     # 24 个工具
  scene/                          # 7 个工具
  scripts/                        # 13 个工具（8 GD + 5 C#）
  filesystem/                     # 12 个工具
  workspace/                      # 13 个工具
  animation/                      # 10 个工具
  settings/                       # 4 个工具
  shader/                         # 5 个工具
  control/                        # 4 个工具
  inputmap/                       # 4 个工具
  export/                         # 4 个工具
  docs/                           # 8 个工具
  audio/                          # 3 个工具
  navigation/                     # 3 个工具
  3d_scene/                       # 3 个工具
  tilemap/                        # 3 个工具
  plugin/                         # 2 个工具
  collision/                      # 1 个工具
  scaffold/                       # 1 个工具
  visualizer/                     # 1 个工具

runtime_tools/
  bridge/                         # 8 个工具
  lifecycle/                      # 6 个工具
```

## 查找工具

使用 `find_tool` 元工具按名称、描述或分类搜索：

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"find_tool","arguments":{"query":"animation"}}}'
```

使用 `get_tools` 列出所有工具，或使用 `get_categories` 浏览分类树。

## 工具工作原理

每个工具遵循相同的模式：

1. **输入**：JSON-RPC `tools/call`，包含工具名称和参数
2. **执行**：服务器运行工具并返回结果
3. **输出**：`{success: true, data: {...}}` 或 `{success: false, error: {code, message}}`

所有工具都有一个**输入模式（input schema）**，描述其期望的参数。客户端使用此模式自动生成工具调用表单。