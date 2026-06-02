# 命令路由

## 完整的调用链路

```
AI 客户端 HTTP POST /mcp {"method": "tools/call", "params": {"name": "get_node_position", "arguments": {...}}}
  │
  ▼
godot_mcp_gdext / http_server.cpp → handle_post()
  │
  ├─ 验证 MCP-Protocol-Version / Content-Type / Accept / Origin
  ├─ 解析 JSON-RPC 2.0 消息
  │
  ▼
mcp_handler.cpp → handle_tools_call()
  │
  ├─ HandlerRegistry::execute(name, args)
  │   ├─ 先查 itool_table_（ITool 对象表，codegen 注册的 124 个工具）
  │   │   └─ 执行 ITool::execute(args)
  │   │       ├─ 前置检查（is_editor_hint, needs_scene, needs_node）
  │   │       └─ execute_impl(ctx) ← 主线程同步
  │   └─ 未命中则查 table_（CommandFn 后备表，当前为空）
  │       └─ 执行 CommandFn(args)
  │
  ▼ (返回值 Dictionary)
  包装为 MCP content array: [{"type":"text","text":...}]
  │
  ▼
HTTP 200 application/json + MCP-Session-Id header
  │
  ▼
AI 客户端
```

## HandlerRegistry 实现

- **统一调度**：`HandlerRegistry::execute()` 先查 `itool_table_`（`std::map<String, unique_ptr<ITool>>`），再查 `table_`（`HashMap<String, CommandFn>` 后备）
- **ITool 注册**：`register_tool(unique_ptr<ITool>)` 存入 `itool_table_`
- **CommandFn 注册**：`register_tool(name, fn)` 存入 `table_`（仅 SDK 自定义工具使用）
- `register_all_tools()` → 委托给 `register_itools()`（由 `tools/codegen.py` 自动生成）
- 普通 `.hpp` 新工具只需 `// @tool register` 注释，**无需**修改 CMakeLists.txt 或 handler_registry.cpp

## 17 组 ITool 分类与 category remap

ITool 的原始 category 通过 `category_remap()` 自动映射到 6 个顶级分类：

| 原始 category | 映射后路径 | 工具数 | 说明 |
|---------------|-----------|:------:|------|
| `meta` | `other/meta` | 5 | 始终可见的元工具 |
| `node` | `node/operation` | 21 | 节点 CRUD |
| `property/2d` | `node/property/2d` | 21 | 2D 属性 |
| `property/3d` | `node/property/3d` | 6 | 3D 属性 |
| `collision` | `node/collision` | 2 | 碰撞体 |
| `find` | `node/find` | 4 | 节点搜索 |
| `scene` | `scene` | 16 | 场景操作 |
| `editor_control` | `editor/general` | 7 | 编辑器控制 |
| `search` | `editor/search` | 3 | 搜索替换 |
| `script_gd` | `script/gdscript` | 5 | GDScript |
| `script/csharp` | `script/csharp` | 6 | C# 脚本 |
| `script_helpers` | `script/helpers` | 3 | 脚本辅助 |
| `settings/core` | `settings/project` | 7 | 项目设置 |
| `settings/extended` | `settings/extended` | 10 | 扩展设置 |
| `input_map` | `settings/input_map` | 4 | 输入映射 |
| `plugin_management` | `other/plugin_management` | 2 | 插件管理 |
| `undo` | `other/undo` | 2 | 撤销/重做 |
| **总计** | | **124** | |

## 两轴分类系统

- **source()**（meta / built_in / custom）：决定可见性。meta 工具始终可见（渐进式披露第一步）
- **category()**：决定分组。原始 category 通过 `category_remap()` 映射到 6 个顶级（node / scene / editor / script / settings / other）

## 注意事项

- 所有命令在主线程同步执行，`process_frame` 驱动的 `HttpServer::poll()` 架构
- 添加新工具时仅需创建 `.hpp` 文件 + `// @tool register` 注释，运行 `tools/codegen.py` 自动注册
- SDK 自定义工具通过 `McpToolRegistry` 注册，自动加 `custom_` 前缀
