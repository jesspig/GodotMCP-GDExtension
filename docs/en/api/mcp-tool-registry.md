# McpToolRegistry

**Inherits**: `Object`

**Singleton**: Access via `Engine.get_singleton("McpToolRegistry")`

Engine singleton that manages all custom tools registered via the SDK. Supports two registration modes: from an `McpToolDefinition` instance (Mode A) or from a `Callable` handler (Mode B).

## Description

`McpToolRegistry` is the central registry for custom MCP tools defined in GDScript or C#. It wraps custom tools into the `HandlerRegistry` dispatch table, making them available alongside the 153 built-in tools. All custom tool names are automatically prefixed with `custom_` to avoid conflicts.

## Methods

| Return | Signature |
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

## Signals

### `tool_registered(name: String)`

Emitted when a custom tool is registered. `name` includes the `custom_` prefix.

### `tool_unregistered(name: String)`

Emitted when a custom tool is unregistered.

## Method Descriptions

### `get_singleton()`
* Returns: `McpToolRegistry`

Returns the global singleton. Equivalent to `Engine.get_singleton("McpToolRegistry")`.

```gdscript
var registry = McpToolRegistry.get_singleton()
```

### `register_tool(name, category, brief, description, input_schema, handler, is_meta, supports_undo, is_destructive)`
* Returns: `void`

Register a custom tool with a `Callable` handler (Mode B). The `name` is automatically prefixed with `custom_`.

**Parameters**: `name: String`, `category: String`, `brief: String`, `description: String`, `input_schema: Dictionary`, `handler: Callable`, `is_meta: bool = false`, `supports_undo: bool = false`, `is_destructive: bool = false`

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
* Returns: `void`

Simplified registration. Equivalent to `register_tool` with description = brief and all flags = false.

### `register_definition(tool_def: McpToolDefinition)`
* Returns: `void`

Register from an `McpToolDefinition` instance (Mode A). Called internally by `McpToolDefinition.register_tool()`.

### `unregister_tool(name: String)`
* Returns: `bool`

Unregister a Mode B tool. Returns `true` if found and removed.

### `unregister_definition(name: String)`
* Returns: `bool`

Unregister a Mode A tool. Returns `true` if found and removed.

### `has_tool(name: String)`
* Returns: `bool`

Check if a custom tool with the given name is registered. Name resolution adds `custom_` prefix if needed.

### `get_custom_tools()`
* Returns: `Array` of `Dictionary`

Returns metadata for all registered custom tools. Each entry contains `name`, `category`, `brief`, `description`, `input_schema`.

### `get_custom_tool_count()`
* Returns: `int`

Number of currently registered custom tools.

## C# API

**Singleton**: `Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>()`

### Properties

| Type | Name | Description |
|------|------|-------------|
| `int` | `CustomToolCount` | Number of registered custom tools |

### Methods

```csharp
registry.RegisterTool(name, category, brief, description, inputSchema, handler, isMeta, supportsUndo, isDestructive);
registry.RegisterToolSimple(name, category, brief, inputSchema, handler);
registry.UnregisterTool(name);
registry.HasTool(name);
```

### Example

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

## Tool Naming

| User registers | Internal name | Clients call |
|----------------|---------------|--------------|
| `"my_tool"` | `"custom_my_tool"` | `custom_my_tool` |
| `"custom_my_tool"` | `"custom_my_tool"` | `custom_my_tool` |

## Best Practices

1. **Always set `input_schema`** for parameter validation
2. **Use descriptive categories** like `"custom/utilities/text"`
3. **Set `is_destructive`** for irreversible operations
4. **Set `supports_undo`** when possible
5. **Register in `_EnterTree`**, unregister in `_ExitTree`