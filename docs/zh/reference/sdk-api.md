# 自定义 MCP 工具（SDK）

GodotMCP 提供了从 GDScript 和 C# 创建自定义 MCP 工具的 SDK。SDK 围绕两个核心类构建：`McpToolDefinition`（可继承的工具基类）和 `McpToolRegistry`（Engine 单例注册中心）。

## McpToolDefinition

**继承自：** `RefCounted`

在 GDScript 中创建自定义工具的基类。覆写 `execute` 实现工具逻辑，然后调用 `register_tool()` 使工具可用。

### 属性

| 类型 | 名称 | 默认值 |
|------|------|--------|
| `String` | `tool_name` | `""` |
| `String` | `category` | `""` |
| `String` | `brief` | `""` |
| `String` | `description` | `""` |
| `Dictionary` | `input_schema` | `{}` |
| `bool` | `is_meta` | `false` |
| `bool` | `supports_undo` | `false` |
| `bool` | `is_destructive` | `false` |

### 方法

| 返回类型 | 签名 |
|----------|------|
| `Dictionary` | `execute(args: Dictionary)` |
| `void` | `register_tool()` |
| `void` | `unregister_tool()` |

### 属性说明

#### `tool_name`
* `String` **tool_name**
  * 默认值：`""`
  * Setter：`set_tool_name(value)`
  * Getter：`get_tool_name()`

唯一工具标识符。注册时自动添加 `custom_` 前缀。作为客户端调用的 MCP 工具名称。

#### `category`
* `String` **category**
  * 默认值：`""`
  * Setter：`set_category(value)`
  * Getter：`get_category()`

组织工具的分类路径。使用斜杠分隔的路径，如 `"custom/utilities/text"`。工具会在分类树中归入指定路径。

#### `brief`
* `String` **brief**
  * 默认值：`""`
  * Setter：`set_brief(value)`
  * Getter：`get_brief()`

工具的单行描述。显示在工具目录中，帮助客户端理解工具的用途。

#### `description`
* `String` **description**
  * 默认值：`""`
  * Setter：`set_description(value)`
  * Getter：`get_description()`

工具的详细使用说明。可包含参数说明、使用注意事项和示例。客户端检查工具时显示。

#### `input_schema`
* `Dictionary` **input_schema**
  * 默认值：`{}`
  * Setter：`set_input_schema(value)`
  * Getter：`get_input_schema()`

描述期望参数的 JSON Schema。必须遵循 `{"type": "object", "properties": {...}, "required": [...]}` 结构。启用参数验证和类型化参数。

#### `is_meta`
* `bool` **is_meta**
  * 默认值：`false`
  * Setter：`set_is_meta(value)`
  * Getter：`get_is_meta()`

如果为 `true`，将此工具标记为元工具，即使没有打开场景也能使用。元工具始终在工具列表中显示。

#### `supports_undo`
* `bool` **supports_undo**
  * 默认值：`false`
  * Setter：`set_supports_undo(value)`
  * Getter：`get_supports_undo()`

如果为 `true`，工具集成 Godot 的撤销/重做系统，以提供更好的用户体验。适用于以可撤销方式修改场景的工具。

#### `is_destructive`
* `bool` **is_destructive**
  * 默认值：`false`
  * Setter：`set_is_destructive(value)`
  * Getter：`get_is_destructive()`

如果为 `true`，工具执行不可逆操作。客户端可能会在调用破坏性工具前显示确认对话框。

### 方法说明

#### `execute(args: Dictionary)`
  * 返回：`Dictionary`

**虚方法** — 覆写此方法以实现工具逻辑。

`args` 包含 MCP 客户端传入的参数，已根据 `input_schema` 进行验证。

返回以下两种格式之一的 `Dictionary`：
- 成功：`{ "success": true, "data": { ... } }`
- 错误：`{ "success": false, "error": { "code": "ERROR_CODE", "message": "可读的错误信息" } }`

```gdscript
# 覆写示例
func execute(args: Dictionary) -> Dictionary:
    var name = args.get("name", "World")
    return {
        "success": true,
        "data": {
            "greeting": "Hello, " + name + "!"
        }
    }
```

#### `register_tool()`
  * 返回：`void`

向 `McpToolRegistry` 单例注册此工具。调用后，工具将以 `custom_<tool_name>` 的名称出现在 MCP 工具目录中。

在配置完属性后调用一次，通常放在 `_init()` 或 `_EnterTree()` 中：

```gdscript
var tool = preload("res://addons/my_tool/hello_world.gd").new()
tool.register_tool()
```

#### `unregister_tool()`
  * 返回：`void`

从 `McpToolRegistry` 单例注销此工具。工具将从 MCP 工具目录中移除，不再可调用。在 `_ExitTree()` 中调用以清理资源。

---

## McpToolRegistry

**继承自：** `Object`

**单例：** 通过 `Engine.get_singleton("McpToolRegistry")` 访问

管理所有通过 SDK 注册的自定义工具的 Engine 单例。支持两种注册模式：模式 A（通过 `McpToolDefinition` 实例）和模式 B（通过 `Callable` 处理器）。GDScript 和 C# 均可访问。

### 方法

| 返回类型 | 签名 |
|----------|------|
| `McpToolRegistry` | `get_singleton()` |
| `void` | `register_tool(name: String, category: String, brief: String, description: String, input_schema: Dictionary, handler: Callable, is_meta: bool = false, supports_undo: bool = false, is_destructive: bool = false)` |
| `void` | `register_tool_simple(name: String, category: String, brief: String, input_schema: Dictionary, handler: Callable)` |
| `void` | `register_definition(tool_def: McpToolDefinition)` |
| `bool` | `unregister_tool(name: String)` |
| `bool` | `unregister_definition(name: String)` |
| `bool` | `has_tool(name: String)` |
| `Array` | `get_custom_tools()` |
| `int` | `get_custom_tool_count()` |

### 信号

#### `tool_registered(name: String)`

当自定义工具注册时发出。`name` 为注册后的工具名称（含 `custom_` 前缀）。

#### `tool_unregistered(name: String)`

当自定义工具注销时发出。`name` 为注册后的工具名称（含 `custom_` 前缀）。

### 方法说明

#### `get_singleton()`
  * 返回：`McpToolRegistry`

返回全局 `McpToolRegistry` 单例。等同于 `Engine.get_singleton("McpToolRegistry")`。

```gdscript
var registry = McpToolRegistry.get_singleton()
```

#### `register_tool(name: String, category: String, brief: String, description: String, input_schema: Dictionary, handler: Callable, is_meta: bool = false, supports_undo: bool = false, is_destructive: bool = false)`
  * 返回：`void`

使用 `Callable` 处理器注册自定义工具（模式 B）。`name` 会自动添加 `custom_` 前缀（除非已存在）。

`handler` 接收一个 `Dictionary` 参数，必须返回 `ToolResult` 格式的 `Dictionary`（`{success: true, data: {...}}` 或 `{success: false, error: {code, message}}`）。

```gdscript
var registry = Engine.get_singleton("McpToolRegistry")
registry.register_tool(
    "my_helper",           # 变为 "custom_my_helper"
    "custom/utilities",    # 分类路径
    "一个辅助工具",         # 简要描述
    "详细的帮助文本",       # 完整描述
    {                      # input_schema
        "type": "object",
        "properties": {
            "value": {"type": "integer", "description": "输入值"}
        }
    },
    Callable(self, "_on_my_helper")
)

func _on_my_helper(args: Dictionary) -> Dictionary:
    return {
        "success": true,
        "data": {"result": args.get("value", 0) * 2}
    }
```

#### `register_tool_simple(name: String, category: String, brief: String, input_schema: Dictionary, handler: Callable)`
  * 返回：`void`

简化注册变体。等同于 `register_tool`，但使用 `description = brief`、`is_meta = false`、`supports_undo = false`、`is_destructive = false`。

```gdscript
registry.register_tool_simple(
    "quick_tool",
    "custom",
    "快速工具",
    {"type": "object", "properties": {}},
    Callable(self, "_on_quick")
)
```

#### `register_definition(tool_def: McpToolDefinition)`
  * 返回：`void`

从 `McpToolDefinition` 实例注册工具（模式 A）。由 `McpToolDefinition.register_tool()` 内部调用。应优先使用定义对象的 `register_tool()` 方法而非直接调用此方法。

#### `unregister_tool(name: String)`
  * 返回：`bool`

注销通过模式 B（`register_tool` 或 `register_tool_simple`）注册的自定义工具。如果工具被找到并移除则返回 `true`。`name` 在匹配时会自动添加 `custom_` 前缀。

```gdscript
registry.unregister_tool("my_helper")
```

#### `unregister_definition(name: String)`
  * 返回：`bool`

注销通过模式 A（`register_definition`）注册的自定义工具。由 `McpToolDefinition.unregister_tool()` 内部调用。如果工具被找到并移除则返回 `true`。

#### `has_tool(name: String)`
  * 返回：`bool`

如果已注册指定 `name` 的自定义工具则返回 `true`。`name` 在匹配时会自动添加 `custom_` 前缀。

```gdscript
if registry.has_tool("my_helper"):
    print("工具已注册为: custom_my_helper")
```

#### `get_custom_tools()`
  * 返回：`Array`，元素类型为 `Dictionary`

返回描述所有已注册自定义工具的 `Dictionary` 数组。每个条目包含 `name`、`category`、`brief`、`description` 和 `input_schema` 键。

```gdscript
for tool_dict in registry.get_custom_tools():
    print(tool_dict.name, " (", tool_dict.category, ")")
```

#### `get_custom_tool_count()`
  * 返回：`int`

返回当前已注册的自定义工具数量。

```gdscript
print("自定义工具数量: ", registry.get_custom_tool_count())
```

---

## C# API

**单例：** `Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>()`

在 C# 中，遵循 `get_` 前缀约定的 `McpToolRegistry` 方法会以属性的形式暴露。通过 `Engine.GetSingleton` 访问单例，使用 `Callable.From` 创建处理器。

### 属性（C# 绑定）

| 类型 | 名称 | 说明 |
|------|------|------|
| `int` | `CustomToolCount` | 已注册的自定义工具数量。映射到 `get_custom_tool_count()`。 |

### 方法（C# 绑定）

| 返回类型 | 签名 |
|----------|------|
| `void` | `RegisterTool(string name, string category, string brief, string description, Dictionary inputSchema, Callable handler, bool isMeta = false, bool supportsUndo = false, bool isDestructive = false)` |
| `void` | `RegisterToolSimple(string name, string category, string brief, Dictionary inputSchema, Callable handler)` |
| `bool` | `UnregisterTool(string name)` |
| `bool` | `HasTool(string name)` |
| `Array<Dictionary>` | `GetCustomTools()` |

### 方法说明

C# 签名遵循 PascalCase 命名约定。所有方法的行为与上述 GDScript 对应方法完全相同。

```csharp
using Godot;

public partial class MyPlugin : EditorPlugin
{
    public override void _EnterTree()
    {
        var registry = Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>();

        // 使用 Callable 注册
        registry.RegisterTool(
            "hello_cs",
            "custom",
            "C# 问候工具",
            "从 C# 注册的工具",
            new Godot.Collections.Dictionary
            {
                ["type"] = "object",
                ["properties"] = new Godot.Collections.Dictionary
                {
                    ["name"] = new Godot.Collections.Dictionary
                    {
                        ["type"] = "string",
                        ["description"] = "要问候的用户名"
                    }
                }
            },
            Callable.From((Godot.Collections.Dictionary args) =>
            {
                var name = args.ContainsKey("name") ? args["name"].AsString() : "World";
                return new Godot.Collections.Dictionary
                {
                    ["success"] = true,
                    ["data"] = new Godot.Collections.Dictionary
                    {
                        ["greeting"] = $"Hello, {name}!"
                    }
                };
            })
        );

        // 查询
        bool exists = registry.HasTool("hello_cs");
        int count = registry.CustomToolCount;
    }
}
```

---

## 工具命名

所有自定义工具自动添加 `custom_` 前缀，以避免与内置工具冲突：

| 用户注册 | 内部名称 | 客户端调用 |
|----------|----------|------------|
| `"my_tool"` | `"custom_my_tool"` | `"custom_my_tool"` |
| `"custom_my_tool"` | `"custom_my_tool"` | `"custom_my_tool"` |

如果名称已以 `custom_` 开头，则不重复添加前缀。

---

## 内置工具集成

自定义工具完全集成到内置工具系统中：

- 出现在 `get_tools` 和 `find_tool` 结果中
- 在分类树中归属其指定的分类
- 支持相同的 `ToolResult` 格式（`{success, data}` 或 `{success, error}`）
- 如设置了 `is_destructive` 标志，会获得相应处理

---

## 最佳实践

1. **始终设置 `input_schema`** — 启用参数验证和类型化参数
2. **使用描述性分类** — 嵌套路径如 `"custom/utilities/text"` 有助于组织工具
3. **对不可逆操作设置 `is_destructive`** — 客户端可能显示确认对话框
4. **尽可能设置 `supports_undo`** — 集成 Godot 撤销系统，提升用户体验
5. **在 `_EnterTree` 注册 / `_ExitTree` 注销** — 与编辑器插件生命周期匹配
6. **简单工具使用 `Callable` 模式** — 避免创建单独的脚本文件
7. **用 curl 通过 `find_tool` 验证** — `curl -X POST http://localhost:9600/mcp -H "Content-Type: application/json" -d '{"method":"tools/call","params":{"name":"find_tool","arguments":{"query":"custom_"}}}'` 确认注册成功
