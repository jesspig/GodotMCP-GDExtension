# cmd_utils -- Shared Tool Templates

7 utility headers that provide reusable patterns and templates for building ITool implementations.

## SchemaBuilder

**File**: `cmd_utils/schema_builder.hpp`

Fluent builder for constructing JSON Schema dictionaries.

```cpp
auto schema = SchemaBuilder()
    .param("parent_path", SchemaBuilder::TYPE_STRING, "Parent node path")
        .required()
    .param("class_name", SchemaBuilder::TYPE_STRING, "Node class name")
        .required()
    .param("node_name", SchemaBuilder::TYPE_STRING, "Name for the node")
    .param("index", SchemaBuilder::TYPE_INT, "Child position index", -1)
    .build();
```

### Methods

| Return | Method | Description |
|--------|--------|-------------|
| `SchemaBuilder&` | `param(name, type, description)` | Add a parameter |
| `SchemaBuilder&` | `required()` | Mark the last param as required |
| `Dictionary` | `build()` | Finalize and return the schema |

### Supported Types

| Constant | Godot Type |
|----------|------------|
| `TYPE_STRING` | `String` |
| `TYPE_INT` | `int` |
| `TYPE_FLOAT` | `float` |
| `TYPE_BOOL` | `bool` |
| `TYPE_ARRAY` | `Array` |
| `TYPE_OBJECT` | `Dictionary` |

---

## DispatchMap

**File**: `cmd_utils/dispatch_map.hpp`

Compile-time string-to-string lookup. Replaces chains of `if/else if` for schema-inferred dispatch.

```cpp
static const DispatchMap<3> NODE_TYPE_MAP = {{
    {"2d", "Node2D"},
    {"3d", "Node3D"},
    {"ui", "Control"},
}};
```

---

## UndoHelpers

**File**: `cmd_utils/undo_helpers.hpp`

Unified undo/redo pattern for tool implementations.

```cpp
undoable_set(node, "position", new_value, old_value);
```

Automatically records before/after values in `EditorUndoRedoManager`.

---

## ArgsGetTyped

**File**: `cmd_utils/args_get_typed.hpp`

Type-safe argument extraction with default values.

```cpp
String parent_path = args_get<String>(args, "parent_path");
int index = args_get<int>(args, "index", -1);
bool enabled = args_get<bool>(args, "enabled");
```

Throws a `Dictionary` error (matching ToolResult format) on type mismatch.

---

## ErrorCodes

**File**: `cmd_utils/error_codes.hpp`

Standard error code constants used across all tools.

| Constant | Value | Description |
|----------|-------|-------------|
| `ERR_TOOL_NOT_FOUND` | `"TOOL_NOT_FOUND"` | Tool name not found |
| `ERR_SCENE_REQUIRED` | `"SCENE_REQUIRED"` | No open scene |
| `ERR_NODE_NOT_FOUND` | `"NODE_NOT_FOUND"` | Node path not found |
| `ERR_INVALID_PARAM` | `"INVALID_PARAM"` | Invalid parameter |
| `ERR_TIMEOUT` | `"TIMEOUT"` | Operation timed out |
| `ERR_INTERNAL` | `"INTERNAL_ERROR"` | Internal error |

---

## MemdeleteGuard

**File**: `cmd_utils/memdelete_guard.hpp`

RAII guard for safe `memdelete`. Ensures Godot objects are freed even if an exception occurs.

```cpp
auto new_node = memnew(Node2D);
MemdeleteGuard guard(new_node);
// If anything throws here, new_node is safely freed
guard.release(); // Transfer ownership to scene tree
```

---

## TrackedSettings

**File**: `cmd_utils/tracked_settings.hpp`

Tracks project setting modifications for batch restore.

```cpp
TrackedSettings settings;
settings.track("application/config/name");
settings.set("application/config/name", "New Name");
// ... later ...
settings.restore(); // Reverts all tracked changes
```