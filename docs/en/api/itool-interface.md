# ITool Interface

Abstract interface for all built-in MCP tools. Each tool inherits from `ITool` and implements virtual methods for metadata and execution logic.

## Description

`ITool` is the base class for all 153 built-in tools. It provides a template method pattern: `execute()` handles schema validation, context resolution, and error wrapping, then delegates to `execute_impl()` for tool-specific logic.

## Methods

| Return | Signature | Type |
|--------|-----------|------|
| `String` | `name()` | Pure virtual |
| `String` | `brief()` | Pure virtual |
| `String` | `description()` | Virtual (default = `brief()`) |
| `String` | `category()` | Pure virtual |
| `String` | `category_description()` | Virtual |
| `Dictionary` | `build_input_schema()` | Virtual |
| `bool` | `is_meta()` | Virtual (default = `false`) |
| `bool` | `needs_scene()` | Virtual (default = `false`) |
| `bool` | `needs_node()` | Virtual (default = `false`) |
| `bool` | `supports_undo()` | Virtual (default = `false`) |
| `bool` | `is_destructive()` | Virtual |
| `void` | `set_registry(HandlerRegistry*)` | Virtual |
| `Dictionary` | `execute(args)` | Non-virtual (template method) |

## Method Descriptions

### `name()`
* Returns: `String`

Unique tool identifier (e.g., `"add_node"`, `"get_scene_tree"`). Used as the MCP tool name.

### `brief()`
* Returns: `String`

One-line description displayed in tool listings.

### `description()`
* Returns: `String`

Detailed description. Defaults to `brief()` if not overridden.

### `category()`
* Returns: `String`

Category path (e.g., `"editor_tools/scene_tree"`, `"runtime_tools/lifecycle"`). Defines the tool's position in the category tree.

### `build_input_schema()`
* Returns: `Dictionary`

JSON Schema describing the tool's input parameters. Format:
```json
{
  "type": "object",
  "properties": {
    "param_name": { "type": "string", "description": "..." }
  },
  "required": ["param_name"]
}
```

Results are cached in `schema_cache_`.

### `is_meta()`
* Returns: `bool`

If `true`, the tool is always visible in tool listings regardless of editor state.

### `needs_scene()`
* Returns: `bool`

If `true`, the tool checks for an open scene before execution. Returns error if no scene is open.

### `needs_node()`
* Returns: `bool`

If `true`, the tool resolves a `node_path` or `path` parameter from arguments. Returns error if the node is not found.

### `supports_undo()`
* Returns: `bool`

If `true`, tool execution is wrapped in `EditorUndoRedoManager` for undo/redo support.

### `execute(args: Dictionary)`
* Returns: `Dictionary`

**Template method** -- the execution flow:
1. Validate input parameters against `build_input_schema()`
2. Resolve scene root (if `needs_scene()`)
3. Resolve target node (if `needs_node()`)
4. Call `execute_impl(ctx)`
5. Ensure `success` key exists in the returned Dictionary

### `execute_impl(ctx: ToolContext)`
* Returns: `Dictionary`

**Protected virtual** -- override this to implement tool logic. The `ToolContext` provides:
- `root: Node*` -- current edited scene root
- `node: Node*` -- resolved target node (if `needs_node()`)
- `args: Dictionary` -- validated input parameters

## ToolContext

```cpp
struct ToolContext {
    Node *root = nullptr;   // Current edited scene root
    Node *node = nullptr;   // Resolved target node
    Dictionary args;         // Validated input arguments
};
```

## ToolResult

Utility class for creating standard response dictionaries:

```cpp
// Success
ToolResult::ok(data_dictionary);

// Error
ToolResult::err("ERROR_CODE", "Human readable message");
```

### Response Format

```json
// Success
{ "success": true, "data": { ... } }

// Error
{ "success": false, "error": { "code": "ERROR_CODE", "message": "..." } }
```