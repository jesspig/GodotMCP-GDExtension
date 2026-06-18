# 着色器工具

5 个用于创建和管理着色器的工具。

## `create_shader`

**分类**: `editor_tools/shader`  
**描述**: 创建着色器。  
**破坏性**: 否 | **撤销**: 否

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `path` | `string` | 是 | - | 着色器文件路径 |
| `shader_type` | `string` | 否 | "canvas_item" | 着色器类型（canvas_item, spatial, particles） |
| `template` | `string` | 否 | "default" | 模板名称 |

---

## `read_shader`

**分类**: `editor_tools/shader`  
**描述**: 读取着色器源码。  
**破坏性**: 否 | **撤销**: 否

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 着色器文件路径 |

---

## `apply_shader_preset`

**分类**: `editor_tools/shader`  
**描述**: 应用着色器预设。  
**破坏性**: 是 | **撤销**: 是 | **需要节点**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 有着色器材质的节点 |
| `preset` | `string` | 是 | 预设名称 |

---

## `get_shader_uniforms`

**分类**: `editor_tools/shader`  
**描述**: 获取着色器 uniform 参数。  
**破坏性**: 否 | **撤销**: 否 | **需要节点**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 有着色器材质的节点 |

### 返回值

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

**分类**: `editor_tools/shader`  
**描述**: 设置着色器 uniform 值。  
**破坏性**: 否 | **撤销**: 是 | **需要节点**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 有着色器材质的节点 |
| `uniform_name` | `string` | 是 | Uniform 名称 |
| `value` | variant | 是 | Uniform 值 |