# HandlerRegistry

**Inherits**: `Object`

Central dispatch engine that manages all tools (built-in and custom), search indexing, and category discovery. Not accessible from GDScript -- used internally by the C++ tool system.

## Description

`HandlerRegistry` is the core of the tool dispatch system. It maintains the ITool dispatch table `itool_table_`, the SDK CommandFn sidetable, search index, frequency tracking for search ranking, and the category tree cache.

## Methods

| Return | Signature |
|--------|-----------|
| `void` | `register_tool(ITool, is_custom)` |
| `Dictionary` | `execute(name, args)` |
| `bool` | `unregister_custom_tool(name)` |
| `Array` | `get_categories()` |
| `Array` | `get_tools_in_category(category)` |
| `Array` | `get_always_on_tools()` |
| `Array` | `search_tools(query, category, limit)` |
| `Array` | `get_search_suggestions(prefix, limit)` |
| `void` | `record_tool_call(name)` |
| `int` | `builtin_tool_count()` |
| `int` | `custom_tool_count()` |

## Method Descriptions

### `register_tool(tool: ITool, is_custom: bool)`
* Returns: `void`

Register a tool. Builds the search index and sets up the tool's registry pointer (for meta tools that need back-references).

### `execute(name: String, args: Dictionary)`
* Returns: `Dictionary`

Look up a tool by name and execute it. If the tool supports undo, wraps the execution in `EditorUndoRedoManager`. Records the call in the frequency index for search ranking.

### `get_categories()`
* Returns: `Array`

Builds a recursive category tree from all registered tools. Uses progressive disclosure -- only returns categories, not individual tools. Results are cached and invalidated on register/unregister.

### `search_tools(query: String, category: String, limit: int)`
* Returns: `Array`

Weighted search across tool names and descriptions:
- Exact match: weight 1000
- Prefix match: weight 500
- Token fuzzy: weight 200
- Description: weight 50

Results sorted by call frequency, then weight.

### `get_search_suggestions(prefix: String, limit: int)`
* Returns: `Array`

Prefix-based autocomplete suggestions, sorted by call frequency.

## Internal Flow

```
execute("add_node", args)
  -> record_tool_call("add_node")  // update frequency
  -> lookup tool in itool_table_
  -> if supports_undo:
      EditorUndoRedoManager::create_action("MCP: add_node")
      tool->execute(args)
      commit_action()
  -> else:
      tool->execute(args)
  -> return result Dictionary
```