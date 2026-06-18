# Custom MCP Tools (SDK)

GodotMCP provides an SDK for creating custom MCP tools from GDScript and C#. The SDK is built around two core classes: `McpToolDefinition` (inheritable tool base class) and `McpToolRegistry` (Engine singleton registry).

## McpToolDefinition

**Inherits:** `RefCounted`

Base class for creating custom tools in GDScript. Override `execute` to implement tool logic, then call `register_tool()` to make the tool available.

### Properties

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

### Methods

| Return | Signature |
|--------|-----------|
| `Dictionary` | `execute(args: Dictionary)` |
| `void` | `register_tool()` |
| `void` | `unregister_tool()` |

### Property Descriptions

#### `tool_name`
* `String` **tool_name**
  * Default: `""`
  * Setter: `set_tool_name(value)`
  * Getter: `get_tool_name()`

Unique tool identifier. Automatically prefixed with `custom_` at registration time. Used as the MCP tool name that clients call.

#### `category`
* `String` **category**
  * Default: `""`
  * Setter: `set_category(value)`
  * Getter: `get_category()`

Category path for organizing tools in the MCP tool catalog. Use slash-separated paths like `"custom/utilities/text"`. Tools appear in the category tree under the specified path.

#### `brief`
* `String` **brief**
  * Default: `""`
  * Setter: `set_brief(value)`
  * Getter: `get_brief()`

One-line description of the tool. Displayed in the tool catalog and used by clients to understand the tool's purpose.

#### `description`
* `String` **description**
  * Default: `""`
  * Setter: `set_description(value)`
  * Getter: `get_description()`

Detailed tool usage description. May include parameter explanations, usage notes, and examples. Displayed when clients inspect the tool.

#### `input_schema`
* `Dictionary` **input_schema**
  * Default: `{}`
  * Setter: `set_input_schema(value)`
  * Getter: `get_input_schema()`

JSON Schema describing the expected parameters. Must follow the structure `{"type": "object", "properties": {...}, "required": [...]}`. Enables parameter validation and typed arguments in clients.

#### `is_meta`
* `bool` **is_meta**
  * Default: `false`
  * Setter: `set_is_meta(value)`
  * Getter: `get_is_meta()`

If `true`, marks this tool as a meta tool that is always available even when no scene is open. Meta tools appear in the tool list regardless of editor state.

#### `supports_undo`
* `bool` **supports_undo**
  * Default: `false`
  * Setter: `set_supports_undo(value)`
  * Getter: `get_supports_undo()`

If `true`, the tool integrates with Godot's undo/redo system for better user experience. Set this for tools that modify the scene in an undoable way.

#### `is_destructive`
* `bool` **is_destructive**
  * Default: `false`
  * Setter: `set_is_destructive(value)`
  * Getter: `get_is_destructive()`

If `true`, the tool performs irreversible operations. Clients may show confirmation dialogs before invoking destructive tools.

### Method Descriptions

#### `execute(args: Dictionary)`
  * Returns: `Dictionary`

**Virtual** — override this method to implement tool logic.

`args` contains the parameters passed by the MCP client, validated against `input_schema`.

Return a `Dictionary` in one of two formats:
- Success: `{ "success": true, "data": { ... } }`
- Error: `{ "success": false, "error": { "code": "ERROR_CODE", "message": "Human-readable message" } }`

```gdscript
# Example override
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
  * Returns: `void`

Register this tool with the `McpToolRegistry` singleton. After calling, the tool becomes available in the MCP tool catalog under the name `custom_<tool_name>`.

Call this once after configuring properties, typically in `_init()` or `_EnterTree()`:

```gdscript
var tool = preload("res://addons/my_tool/hello_world.gd").new()
tool.register_tool()
```

#### `unregister_tool()`
  * Returns: `void`

Unregister this tool from the `McpToolRegistry` singleton. The tool is removed from the MCP tool catalog and will no longer be callable. Call in `_ExitTree()` to clean up.

---

## McpToolRegistry

**Inherits:** `Object`

**Singleton:** Access via `Engine.get_singleton("McpToolRegistry")`

Engine singleton that manages all custom tools registered via the SDK. Supports two registration modes: Mode A (from an `McpToolDefinition` instance) and Mode B (from a `Callable` handler). Accessible from both GDScript and C#.

### Methods

| Return | Signature |
|--------|-----------|
| `McpToolRegistry` | `get_singleton()` |
| `void` | `register_tool(name: String, category: String, brief: String, description: String, input_schema: Dictionary, handler: Callable, is_meta: bool = false, supports_undo: bool = false, is_destructive: bool = false)` |
| `void` | `register_tool_simple(name: String, category: String, brief: String, input_schema: Dictionary, handler: Callable)` |
| `void` | `register_definition(tool_def: McpToolDefinition)` |
| `bool` | `unregister_tool(name: String)` |
| `bool` | `unregister_definition(name: String)` |
| `bool` | `has_tool(name: String)` |
| `Array` | `get_custom_tools()` |
| `int` | `get_custom_tool_count()` |

### Signals

#### `tool_registered(name: String)`

Emitted when a custom tool is registered. `name` is the registered tool name (with `custom_` prefix).

#### `tool_unregistered(name: String)`

Emitted when a custom tool is unregistered. `name` is the registered tool name (with `custom_` prefix).

### Method Descriptions

#### `get_singleton()`
  * Returns: `McpToolRegistry`

Returns the global `McpToolRegistry` singleton. Equivalent to `Engine.get_singleton("McpToolRegistry")`.

```gdscript
var registry = McpToolRegistry.get_singleton()
```

#### `register_tool(name: String, category: String, brief: String, description: String, input_schema: Dictionary, handler: Callable, is_meta: bool = false, supports_undo: bool = false, is_destructive: bool = false)`
  * Returns: `void`

Register a custom tool with a `Callable` handler (Mode B). The `name` is automatically prefixed with `custom_` unless already present.

`handler` receives a `Dictionary` of arguments and must return a `Dictionary` in `ToolResult` format (`{success: true, data: {...}}` or `{success: false, error: {code, message}}`).

```gdscript
var registry = Engine.get_singleton("McpToolRegistry")
registry.register_tool(
    "my_helper",           # becomes "custom_my_helper"
    "custom/utilities",    # category path
    "A helper tool",       # brief description
    "Detailed help text",  # full description
    {                      # input_schema
        "type": "object",
        "properties": {
            "value": {"type": "integer", "description": "Input value"}
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
  * Returns: `void`

Simplified registration variant. Equivalent to `register_tool` with `description = brief`, `is_meta = false`, `supports_undo = false`, `is_destructive = false`.

```gdscript
registry.register_tool_simple(
    "quick_tool",
    "custom",
    "A quick tool",
    {"type": "object", "properties": {}},
    Callable(self, "_on_quick")
)
```

#### `register_definition(tool_def: McpToolDefinition)`
  * Returns: `void`

Register a tool from an `McpToolDefinition` instance (Mode A). Called internally by `McpToolDefinition.register_tool()`. Use the definition object's `register_tool()` method instead of calling this directly.

#### `unregister_tool(name: String)`
  * Returns: `bool`

Unregister a custom tool registered via Mode B (`register_tool` or `register_tool_simple`). Returns `true` if the tool was found and removed. The `name` is matched after adding the `custom_` prefix if not already present.

```gdscript
registry.unregister_tool("my_helper")
```

#### `unregister_definition(name: String)`
  * Returns: `bool`

Unregister a custom tool registered via Mode A (`register_definition`). Called internally by `McpToolDefinition.unregister_tool()`. Returns `true` if the tool was found and removed.

#### `has_tool(name: String)`
  * Returns: `bool`

Returns `true` if a custom tool with the given `name` is registered. The `name` is matched after adding the `custom_` prefix if not already present.

```gdscript
if registry.has_tool("my_helper"):
    print("Tool exists as: custom_my_helper")
```

#### `get_custom_tools()`
  * Returns: `Array` of `Dictionary`

Returns an array of `Dictionary` entries describing all registered custom tools. Each entry contains `name`, `category`, `brief`, `description`, and `input_schema` keys.

```gdscript
for tool_dict in registry.get_custom_tools():
    print(tool_dict.name, " (", tool_dict.category, ")")
```

#### `get_custom_tool_count()`
  * Returns: `int`

Returns the number of currently registered custom tools.

```gdscript
print("Custom tool count: ", registry.get_custom_tool_count())
```

---

## C# API

**Singleton:** `Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>()`

In C#, `McpToolRegistry` methods following the `get_` prefix convention are exposed as properties. Access the singleton via `Engine.GetSingleton` and use `Callable.From` for handler creation.

### Properties (C# Binding)

| Type | Name | Description |
|------|------|-------------|
| `int` | `CustomToolCount` | Number of registered custom tools. Maps to `get_custom_tool_count()`. |

### Methods (C# Binding)

| Return | Signature |
|--------|-----------|
| `void` | `RegisterTool(string name, string category, string brief, string description, Dictionary inputSchema, Callable handler, bool isMeta = false, bool supportsUndo = false, bool isDestructive = false)` |
| `void` | `RegisterToolSimple(string name, string category, string brief, Dictionary inputSchema, Callable handler)` |
| `bool` | `UnregisterTool(string name)` |
| `bool` | `HasTool(string name)` |
| `Array<Dictionary>` | `GetCustomTools()` |

### Method Descriptions

C# signatures follow PascalCase conventions. All methods behave identically to their GDScript counterparts described above.

```csharp
using Godot;

public partial class MyPlugin : EditorPlugin
{
    public override void _EnterTree()
    {
        var registry = Engine.GetSingleton("McpToolRegistry").As<McpToolRegistry>();

        // Register with Callable
        registry.RegisterTool(
            "hello_cs",
            "custom",
            "C# hello tool",
            "A tool registered from C#",
            new Godot.Collections.Dictionary
            {
                ["type"] = "object",
                ["properties"] = new Godot.Collections.Dictionary
                {
                    ["name"] = new Godot.Collections.Dictionary
                    {
                        ["type"] = "string",
                        ["description"] = "Name to greet"
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

        // Query
        bool exists = registry.HasTool("hello_cs");
        int count = registry.CustomToolCount;
    }
}
```

---

## Tool Naming

All custom tools are automatically prefixed with `custom_` to avoid conflicts with built-in tools:

| User registers | Internal name | Client calls |
|----------------|---------------|--------------|
| `"my_tool"` | `"custom_my_tool"` | `"custom_my_tool"` |
| `"custom_my_tool"` | `"custom_my_tool"` | `"custom_my_tool"` |

If the name already starts with `custom_`, no additional prefix is added.

---

## Built-in Tool Interaction

Custom tools are fully integrated with the built-in tool system:

- Listed in `get_tools` and `find_tool` results
- Appear in the category tree under their assigned category
- Support the same `ToolResult` format (`{success, data}` or `{success, error}`)
- Respect the `is_destructive` flag if set

---

## Best Practices

1. **Always set `input_schema`** — this enables parameter validation and typed arguments
2. **Use descriptive categories** — nested paths like `"custom/utilities/text"` help organize tools
3. **Set `is_destructive` for irreversible operations** — clients may show confirmation dialogs
4. **Set `supports_undo` when possible** — integrates with Godot's undo system for better UX
5. **Register in `_EnterTree` / unregister in `_ExitTree`** — match the editor plugin lifecycle
6. **Use `Callable` mode for simple tools** — avoids needing a separate script file
7. **Test with `find_tool` in curl** — `curl -X POST http://localhost:9600/mcp -H "Content-Type: application/json" -d '{"method":"tools/call","params":{"name":"find_tool","arguments":{"query":"custom_"}}}'` to verify registration
