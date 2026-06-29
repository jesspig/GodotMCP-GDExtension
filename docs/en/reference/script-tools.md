# Script Tools

13 tools for reading, writing, and managing GDScript and C# scripts.

## GDScript Tools (8)

### `read_gd_script`

**Category**: `editor_tools/scripts`  
**Description**: Read a GDScript file.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Script file path |

#### Returns

```json
{ "success": true, "data": { "content": "extends Node\n\nfunc _ready():\n    pass\n", "path": "res://player.gd" } }
```

---

### `write_gd_script`

**Category**: `editor_tools/scripts`  
**Description**: Write or create a GDScript file.  
**Destructive**: Yes | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Script file path |
| `content` | `string` | Yes | Script content |

---

### `patch_gd_script`

**Category**: `editor_tools/scripts`  
**Description**: Patch a GDScript file (find and replace).  
**Destructive**: Yes | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Script file path |
| `old_string` | `string` | Yes | Text to replace |
| `new_string` | `string` | Yes | Replacement text |

---

### `validate_gd_script`

**Category**: `editor_tools/scripts`  
**Description**: Validate GDScript syntax via LSP.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Script file path |

#### Returns

```json
{ "success": true, "data": { "valid": true } }
```

Or with errors:

```json
{ "success": true, "data": { "valid": false, "errors": [{"line": 5, "message": "Unexpected token"}] } }
```

---

### `list_gd_scripts`

**Category**: `editor_tools/scripts`  
**Description**: List all GDScript files in the project.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | No | Directory to search (default: res://) |

---

### `grep_scripts`

**Category**: `editor_tools/scripts`  
**Description**: Search text in scripts.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `pattern` | `string` | Yes | Search pattern |
| `include` | `string` | No | File pattern filter |

---

### `glob_scripts`

**Category**: `editor_tools/scripts`  
**Description**: Search script files by pattern.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `pattern` | `string` | Yes | Glob pattern |
| `path` | `string` | No | Search root |

---

### `run_editor_script`

**Category**: `editor_tools/scripts`  
**Description**: Execute an EditorScript (.gd) file in the editor context.  
**Destructive**: Yes | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Path to .gd script file (must extend EditorScript) |
| `timeout_ms` | `integer` | No | Max execution time in milliseconds (default 5000, max 30000) |

#### Returns

```json
{ "success": true, "data": { "path": "res://my_script.gd", "execution_time_ms": 42, "stdout": "Hello from EditorScript!", "timed_out": false } }
```

---

## C# Script Tools (5)

### `read_csharp_script`

**Category**: `editor_tools/scripts`  
**Description**: Read a C# script file.  
**Destructive**: No | **Undo**: No

### `write_csharp_script`

**Category**: `editor_tools/scripts`  
**Description**: Write or create a C# script file.  
**Destructive**: Yes | **Undo**: No

### `patch_csharp_script`

**Category**: `editor_tools/scripts`  
**Description**: Patch a C# script file (find and replace).  
**Destructive**: Yes | **Undo**: No

### `validate_csharp_script`

**Category**: `editor_tools/scripts`  
**Description**: Validate C# script syntax via build.  
**Destructive**: No | **Undo**: No

### `list_csharp_scripts`

**Category**: `editor_tools/scripts`  
**Description**: List all C# script files.  
**Destructive**: No | **Undo**: No

> Parameters mirror their GDScript counterparts above.