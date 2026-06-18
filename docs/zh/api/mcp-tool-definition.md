# McpToolDefinition

**继承**: `RefCounted`

在 GDScript 或 C# 中创建自定义 MCP 工具的基类。重写 `execute` 方法实现工具逻辑，然后调用 `register_tool()` 使工具在 MCP 目录中可用。

## 描述

`McpToolDefinition` 是从脚本创建自定义工具的主要方式。它是一个 `RefCounted` 对象，具有可配置属性和一个虚方法 `execute`。配置并注册后，该工具以 `custom_<tool_name>` 的名称对 MCP 客户端可用。

## 属性

| 类型 | 名称 | 默认值 |
|------|------|---------|
| `String` | `tool_name` | `""` |
| `String` | `category` | `""` |
| `String` | `brief` | `""` |
| `String` | `description` | `""` |
| `Dictionary` | `input_schema` | `{}` |
| `bool` | `is_meta` | `false` |
| `bool` | `supports_undo` | `false` |
| `bool` | `is_destructive` | `false` |

## 方法

| 返回值 | 签名 |
|--------|-----------|
| `Dictionary` | `execute(args: Dictionary)` |
| `void` | `register_tool()` |
| `void` | `unregister_tool()` |

## 属性说明

### `tool_name`
* `String` **tool_name**
  * 默认值: `""`
  * Setter: `set_tool_name(value)`
  * Getter: `get_tool_name()`

唯一的工具标识符。注册时自动添加 `custom_` 前缀。作为客户端调用的 MCP 工具名称。

### `category`
* `String` **category**
  * 默认值: `""`
  * Setter: `set_category(value)`
  * Getter: `get_category()`

用于在 MCP 目录中组织工具的类别路径。使用斜杠分隔的路径，如 `"custom/utilities/text"`。

### `brief`
* `String` **brief**
  * 默认值: `""`
  * Setter: `set_brief(value)`
  * Getter: `get_brief()`

工具的一行描述。显示在工具列表中。

### `description`
* `String` **description**
  * 默认值: `""`
  * Setter: `set_description(value)`
  * Getter: `get_description()`

详细的使用描述。可包含参数说明和示例。

### `input_schema`
* `Dictionary` **input_schema**
  * 默认值: `{}`
  * Setter: `set_input_schema(value)`
  * Getter: `get_input_schema()`

描述预期参数的 JSON Schema。必须遵循: `{"type": "object", "properties": {...}, "required": [...]}`。

### `is_meta`
* `bool` **is_meta**
  * 默认值: `false`
  * Setter: `set_is_meta(value)`
  * Getter: `get_is_meta()`

如果为 `true`，无论编辑器状态如何，该工具都会显示在工具列表中。

### `supports_undo`
* `bool` **supports_undo**
  * 默认值: `false`
  * Setter: `set_supports_undo(value)`
  * Getter: `get_supports_undo()`

如果为 `true`，工具会与 Godot 的撤销/重做系统集成。

### `is_destructive`
* `bool` **is_destructive**
  * 默认值: `false`
  * Setter: `set_is_destructive(value)`
  * Getter: `get_is_destructive()`

如果为 `true`，工具执行不可逆操作。客户端可能会显示确认对话框。

## 方法说明

### `execute(args: Dictionary)`
* 返回: `Dictionary`

**虚方法** -- 重写此方法以实现工具逻辑。`args` 包含经过 `input_schema` 验证的参数。

返回格式:
- 成功: `{ "success": true, "data": { ... } }`
- 错误: `{ "success": false, "error": { "code": "ERROR_CODE", "message": "..." } }`

```gdscript
func execute(args: Dictionary) -> Dictionary:
    var name = args.get("name", "World")
    return { "success": true, "data": { "greeting": "Hello, " + name + "!" } }
```

### `register_tool()`
* 返回: `void`

向 `McpToolRegistry` 注册此工具。调用后，该工具以 `custom_<tool_name>` 的名称可用。

```gdscript
var tool = preload("res://my_tool.gd").new()
tool.tool_name = "hello"
tool.brief = "Says hello"
tool.register_tool()
```

### `unregister_tool()`
* 返回: `void`

注销此工具。在 `_ExitTree()` 中调用以进行清理。