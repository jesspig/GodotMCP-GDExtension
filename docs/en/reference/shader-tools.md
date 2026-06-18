# Shader Tools

5 tools for creating and managing shaders.

## `create_shader`

**Category**: `editor_tools/shader`  
**Description**: Create a shader.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `path` | `string` | Yes | - | Shader file path |
| `shader_type` | `string` | No | "canvas_item" | Shader type (canvas_item, spatial, particles) |
| `template` | `string` | No | "default" | Template name |

---

## `read_shader`

**Category**: `editor_tools/shader`  
**Description**: Read shader source.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Shader file path |

---

## `apply_shader_preset`

**Category**: `editor_tools/shader`  
**Description**: Apply a shader preset.  
**Destructive**: Yes | **Undo**: Yes | **Needs Node**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node with shader material |
| `preset` | `string` | Yes | Preset name |

---

## `get_shader_uniforms`

**Category**: `editor_tools/shader`  
**Description**: Get shader uniform parameters.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node with shader material |

### Returns

```json
{
  "success": true,
  "data": {
    "uniforms": [
      { "name": "u_color", "type": "vec4", "default": [1,1,1,1] },
      { "name": "u_intensity", "type": "float", "default": 1.0 }
    ]
  }
}
```

---

## `set_shader_uniform`

**Category**: `editor_tools/shader`  
**Description**: Set a shader uniform value.  
**Destructive**: No | **Undo**: Yes | **Needs Node**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node with shader material |
| `uniform_name` | `string` | Yes | Uniform name |
| `value` | variant | Yes | Uniform value |