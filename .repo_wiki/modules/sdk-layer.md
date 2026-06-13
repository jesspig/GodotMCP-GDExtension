# SDK 层（GDScript 自定义工具）

> 允许 GDScript 插件通过 `McpToolDefinition` 和 `McpToolRegistry` 注册自定义 MCP 工具。

## 架构

```mermaid
flowchart LR
    subgraph GDScript["GDScript 插件"]
        DEF["extends McpToolDefinition<br/>func execute(args): → Dictionary"]
        REG["register_tool() 注册到单例"]
    end
    subgraph Cpp["C++ GDExtension"]
        REGISTRY["McpToolRegistry<br/>（单例 Object）"]
        ADAPTER["IToolAdapter<br/>（包装 Callable 为 ITool）"]
        HANDLER["HandlerRegistry::itool_table_"]
    end
    DEF -->|"register_tool()"| REGISTRY
    REGISTRY -->|"创建 IToolAdapter"| ADAPTER
    ADAPTER -->|"register_tool(adapter, is_custom=true)"| HANDLER
```

## 两种注册模式

### Mode A：继承 `McpToolDefinition`（RefCounted）

```gdscript
extends McpToolDefinition

func _init():
    tool_name = "my_custom_tool"
    category = "editor_tools/my_category"
    brief = "Does something useful"
    description = "A longer description of what this tool does"
    input_schema = {
        "type": "object",
        "properties": {
            "param1": {"type": "string", "description": "A parameter"}
        },
        "required": ["param1"]
    }
    is_meta = false
    supports_undo = false
    is_destructive = false

func execute(args: Dictionary) -> Dictionary:
    # 业务逻辑
    return {"success": true, "data": {"result": "done"}}
```

- 调用 `register_tool()` 注册到 `McpToolRegistry` 单例
- `execute()` 通过 `call("execute", args)` 动态分发（`mcp_tool_definition.cpp:73`）
- 自动添加 `custom_` 前缀避免与内置工具命名冲突

### Mode B：Callable 注册

```gdscript
McpToolRegistry.get_singleton().register_tool(
    "my_tool",              # name
    "editor_tools/tools",   # category
    "Brief",                # brief
    "Description",          # description
    {"type": "object", "properties": {}},  # input_schema
    Callable(self, "_my_handler"),  # handler
    false                   # is_meta
)
```

## IToolAdapter 适配器

`tool_adapter.hpp` 将 SDK 的 `Callable` 或 `CommandFn` 包装为 `ITool` 接口，存入 `HandlerRegistry::itool_table_` 与内置工具统一调度：

```cpp
class IToolAdapter : public ITool {
    // 从 SDK 元数据返回 name/category/brief/description/input_schema
    // execute_impl() 调 Callable::call(args) 或 CommandFn(args)
};
```

## 关键文件

| 文件 | 用途 |
|------|------|
| `extensions/src/sdk/mcp_tool_definition.hpp/.cpp` | GDScript 可继承的 RefCounted 基类 |
| `extensions/src/sdk/mcp_tool_registry.hpp/.cpp` | 单例注册表，管理自定义工具生命周期 |
| `extensions/src/built_in/tool_adapter.hpp/.cpp` | Callable → ITool 适配器 |

## 注意事项

- 自定义工具名自动加 `custom_` 前缀（`mcp_tool_registry.cpp:65`）
- 重复注册同名工具会覆盖旧工具（发出警告）
- 工具变更通过 `notify_tools_changed()` → `McpHandler::notify_tools_list_changed()` → SSE 推送 `notifications/tools/list_changed` 通知所有已初始化会话
- `McpToolRegistry` 单例在 `register_types.cpp` 的 `LEVEL_EDITOR` 阶段初始化
