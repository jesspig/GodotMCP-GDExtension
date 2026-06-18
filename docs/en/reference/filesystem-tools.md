# Filesystem Tools

12 tools for creating, deleting, and managing project files.

## `create`

**Category**: `editor_tools/filesystem`  
**Description**: Create a file.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | File path |
| `content` | `string` | No | File content |

---

## `create_directory`

**Category**: `editor_tools/filesystem`  
**Description**: Create a directory.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Directory path |
| `recursive` | `bool` | No | Create parent directories |

---

## `create_scene`

**Category**: `editor_tools/filesystem`  
**Description**: Create a scene file.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Scene file path |
| `root_type` | `string` | No | Root node type (default: "Node") |

---

## `create_resource`

**Category**: `editor_tools/filesystem`  
**Description**: Create a resource file.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Resource file path |
| `type` | `string` | Yes | Resource type |

---

## `create_gdshader`

**Category**: `editor_tools/filesystem`  
**Description**: Create a shader file.  
**Destructive**: No | **Undo**: No

---

## `delete_file`

**Category**: `editor_tools/filesystem`  
**Description**: Delete a file.  
**Destructive**: Yes | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | File path to delete |

---

## `move_file`

**Category**: `editor_tools/filesystem`  
**Description**: Move or rename a file.  
**Destructive**: Yes | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `from` | `string` | Yes | Source path |
| `to` | `string` | Yes | Destination path |

---

## `copy_file`

**Category**: `editor_tools/filesystem`  
**Description**: Copy a file.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `from` | `string` | Yes | Source path |
| `to` | `string` | Yes | Destination path |

---

## `open_file`

**Category**: `editor_tools/filesystem`  
**Description**: Open a file in the editor.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | File path to open |

---

## `list_directory`

**Category**: `editor_tools/filesystem`  
**Description**: List directory contents.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Directory path |

---

## `search_files`

**Category**: `editor_tools/filesystem`  
**Description**: Search project files.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `pattern` | `string` | Yes | Search pattern |
| `path` | `string` | No | Root path (default: res://) |

---

## `save_resource_as`

**Category**: `editor_tools/filesystem`  
**Description**: Save a resource as a file.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `resource_path` | `string` | Yes | Source resource path |
| `save_path` | `string` | Yes | Destination file path |