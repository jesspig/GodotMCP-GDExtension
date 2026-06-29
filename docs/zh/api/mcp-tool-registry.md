# McpToolRegistry

**继承**: `Object`

**单例**: 通过 `Engine.get_singleton("McpToolRegistry")` 访问

管理所有通过 SDK 注册的自定义工具的引擎单例。支持两种注册模式：来自 `McpToolDefinition` 实例（模式 A）或来自 `Callable` 处理器（模式 B）。

## 描述

`McpToolRegistry` 是在 GDScript 或 C# 中定义的自定义 MCP 工具的中央注册中心。它将自定义工具包装到 `HandlerRegistry` 调度表中，使其与 164 个内置工具一起可用。所有自定义工具名称自动添加 `custom_` 前缀以避免冲突。

## 方法

| 返回值 | 签名 |
|--------|-----------|
| `McpToolRegistry` | `get_singleton()` |
| `void` | `register_tool(name, category, brief, description, input_schema, handler, is_meta, supports_undo, is_destructive)` |
| `void` | `register_tool_simple(name, category, brief, input_schema, handler)` |
| `void` | `register_definition(tool_def)` |
| `bool` | `unregister_tool(name)` |
| `bool` | `unregister_definition(name)` |
| `bool` | `has_tool(name)` |
| `Array` | `get_custom_tools()` |
| `int` | `get_custom_tool_count()` |

## 信号

### `tool_registered(name: String)`

当自定义工具注册时发出。`name` 包含 `custom_` 前缀。

### `tool_unregistered(name: String)`

当自定义工具注销时发出。

## 方法说明

### `get_singleton()`
* 返回: `McpToolRegistry`

返回全局单例。等同于 `Engine.get_singleton("McpToolRegistry")`。

```gdscript
var registry = McpToolRegistry.get_singleton()
```

### `register_tool(name, category, brief, description, input_schema, handler, is_meta, supports_undo, is_destructive)`
* 返回: `void`

使用 `Callable` 处理器（模式 B）注册自定义工具。`name` 自动添加 `custom_` 前缀。

**参数**: `name: String`, `category: String`, `brief: String`, `description: String`, `input_schema: Dictionary`, `handler: Callable`, `is_meta: bool = false`, `supports_undo: bool = false`, `is_destructive: bool = false`

```gdscript
registry.register_tool(
    "my_helper",
    "custom/utilities",
    "A helper tool",
    "Detailed help text",
    { "type": "object", "properties": {
        "value": { "type": "integer", "description": "Input value" }
    }},
    Callable(self, "_on_my_helper")
)
```

### `register_tool_simple(name, category, brief, input_schema, handler)`
* 返回: `void`

简化注册。等同于 `register_tool`，其中 description = brief，所有标志 = false。

### `register_definition(tool_def: McpToolDefinition)`
* 返回: `void`

通过 `McpToolDefinition` 实例（模式 A）注册。由 `McpToolDefinition.register_tool()` 内部调用。

### `unregister_tool(name: String)`
* 返回: `bool`

注销模式 B 的工具。找到并移除时返回 `true`。

### `unregister_definition(name: String)`
* 返回: `bool`

注销模式 A 的工具。找到并移除时返回 `true`。

### `has_tool(name: String)`
* 返回: `bool`

检查是否已注册具有指定名称的自定义工具。名称解析会在需要时添加 `custom_` 前缀。

### `get_custom_tools()`
* 返回: `Array` of `Dictionary`

返回所有已注册自定义工具的元数据。每个条目包含 `name`, `category`, `brief`, `description`, `input_schema`。

### `get_custom_tool_count()`
* 返回: `int`

当前已注册的自定义工具数量。

## C# API

**单例**: `Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>()`

### 属性

| 类型 | 名称 | 描述 |
|------|------|-------------|
| `int` | `CustomToolCount` | 已注册的自定义工具数量 |

### 方法

```csharp
registry.RegisterTool(name, category, brief, description, inputSchema, handler, isMeta, supportsUndo, isDestructive);
registry.RegisterToolSimple(name, category, brief, inputSchema, handler);
registry.UnregisterTool(name);
registry.HasTool(name);
```

### 示例

```csharp
var registry = Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>();
registry.RegisterTool(
    "hello_cs", "custom", "C# hello tool", "A tool from C#",
    new Godot.Collections.Dictionary { ["type"] = "object", ["properties"] = new Godot.Collections.Dictionary() },
    Callable.From((Godot.Collections.Dictionary args) => {
        return new Godot.Collections.Dictionary { ["success"] = true, ["data"] = new Godot.Collections.Dictionary() };
    })
);
```

## 工具命名

| 用户注册 | 内部名称 | 客户端调用 |
|----------------|---------------|--------------|
| `"my_tool"` | `"custom_my_tool"` | `custom_my_tool` |
| `"custom_my_tool"` | `"custom_my_tool"` | `custom_my_tool` |

## 最佳实践

1. **始终设置 `input_schema`** 用于参数验证
2. **使用描述性类别**，如 `"custom/utilities/text"`
3. **设置 `is_destructive`** 用于不可逆操作
4. **尽可能设置 `supports_undo`**
5. **在 `_EnterTree` 中注册**，在 `_ExitTree` 中注销