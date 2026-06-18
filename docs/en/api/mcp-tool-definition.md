# McpToolDefinition

**Inherits**: `RefCounted`

Base class for creating custom MCP tools in GDScript or C#. Override the `execute` method to implement tool logic, then call `register_tool()` to make the tool available in the MCP catalog.

## Description

`McpToolDefinition` is the primary way to create custom tools from scripts. It is a `RefCounted` object with configurable properties and a virtual `execute` method. Once configured and registered, it becomes available to MCP clients under the name `custom_<tool_name>`.

## Properties

| Type | Name | Default |
|------|------|---------|
| `String` | `tool_name` | `""` |
| `String` | `category` | `""` |
| `String` | `brief` | `""` |
| `String` | `description` | `""` |
| `Dictionary` | `input_schema` | `{}` |
| `bool` | `is_meta` | `false` |
| `bool` | `supports_undo` | `false` |
| `bool` | `is_destructive` | `false` |

## Methods

| Return | Signature |
|--------|-----------|
| `Dictionary` | `execute(args: Dictionary)` |
| `void` | `register_tool()` |
| `void` | `unregister_tool()` |

## Property Descriptions

### `tool_name`
* `String` **tool_name**
  * Default: `""`
  * Setter: `set_tool_name(value)`
  * Getter: `get_tool_name()`

Unique tool identifier. Automatically prefixed with `custom_` at registration time. Used as the MCP tool name that clients call.

### `category`
* `String` **category**
  * Default: `""`
  * Setter: `set_category(value)`
  * Getter: `get_category()`

Category path for organizing tools in the MCP catalog. Use slash-separated paths like `"custom/utilities/text"`.

### `brief`
* `String` **brief**
  * Default: `""`
  * Setter: `set_brief(value)`
  * Getter: `get_brief()`

One-line description of the tool. Displayed in tool listings.

### `description`
* `String` **description**
  * Default: `""`
  * Setter: `set_description(value)`
  * Getter: `get_description()`

Detailed tool usage description. May include parameter explanations and examples.

### `input_schema`
* `Dictionary` **input_schema**
  * Default: `{}`
  * Setter: `set_input_schema(value)`
  * Getter: `get_input_schema()`

JSON Schema describing expected parameters. Must follow: `{"type": "object", "properties": {...}, "required": [...]}`.

### `is_meta`
* `bool` **is_meta**
  * Default: `false`
  * Setter: `set_is_meta(value)`
  * Getter: `get_is_meta()`

If `true`, this tool appears in the tool list regardless of editor state.

### `supports_undo`
* `bool` **supports_undo**
  * Default: `false`
  * Setter: `set_supports_undo(value)`
  * Getter: `get_supports_undo()`

If `true`, the tool integrates with Godot's undo/redo system.

### `is_destructive`
* `bool` **is_destructive**
  * Default: `false`
  * Setter: `set_is_destructive(value)`
  * Getter: `get_is_destructive()`

If `true`, the tool performs irreversible operations. Clients may show confirmation dialogs.

## Method Descriptions

### `execute(args: Dictionary)`
* Returns: `Dictionary`

**Virtual** -- override this method to implement tool logic. `args` contains parameters validated against `input_schema`.

Return format:
- Success: `{ "success": true, "data": { ... } }`
- Error: `{ "success": false, "error": { "code": "ERROR_CODE", "message": "..." } }`

```gdscript
func execute(args: Dictionary) -> Dictionary:
    var name = args.get("name", "World")
    return { "success": true, "data": { "greeting": "Hello, " + name + "!" } }
```

### `register_tool()`
* Returns: `void`

Register this tool with `McpToolRegistry`. After calling, the tool becomes available as `custom_<tool_name>`.

```gdscript
var tool = preload("res://my_tool.gd").new()
tool.tool_name = "hello"
tool.brief = "Says hello"
tool.register_tool()
```

### `unregister_tool()`
* Returns: `void`

Unregister this tool. Call in `_ExitTree()` to clean up.