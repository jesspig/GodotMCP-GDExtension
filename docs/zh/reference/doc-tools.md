# 文档查询工具

8 个用于在运行时查询 Godot ClassDB 文档的工具。

## `search_docs`

**分类**: `editor_tools/docs`  
**描述**: 搜索 Godot 文档。  
**破坏性**: 否 | **撤销**: 否

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `query` | `string` | 是 | 搜索查询 |
| `category` | `string` | 否 | 筛选："class", "method", "property", "signal", "constant" |

### 返回值

```json
{
  "success": true,
  "data": {
    "results": [
      { "name": "Node2D", "type": "class", "description": "Base node for 2D..." },
      { "name": "position", "type": "property", "description": "Position in 2D space..." }
    ]
  }
}
```

---

## `get_class_info`

**分类**: `editor_tools/docs`  
**描述**: 获取完整的类信息。  
**破坏性**: 否 | **撤销**: 否

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `class_name` | `string` | 是 | 类名称 |

### 返回值

```json
{
  "success": true,
  "data": {
    "name": "Sprite2D",
    "inherits": "Node2D",
    "inherits_chain": ["Node2D", "CanvasItem", "Node", "Object"],
    "description": "Sprite2D description from ClassDB...",
    "methods": [...],
    "properties": [...],
    "signals": [...],
    "constants": [...],
    "enums": [...]
  }
}
```

---

## `get_class_list`

**分类**: `editor_tools/docs`  
**描述**: 列出所有 Godot 类。  
**破坏性**: 否 | **撤销**: 否

---

## `get_inheritance_chain`

**分类**: `editor_tools/docs`  
**描述**: 获取类继承链。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `class_name` | `string` | 是 | 类名称 |

---

## `get_property_doc`

**分类**: `editor_tools/docs`  
**描述**: 查询属性文档。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `class_name` | `string` | 是 | 类名称 |
| `property_name` | `string` | 是 | 属性名称 |

---

## `get_method_doc`

**分类**: `editor_tools/docs`  
**描述**: 查询方法文档。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `class_name` | `string` | 是 | 类名称 |
| `method_name` | `string` | 是 | 方法名称 |

---

## `get_enum_doc`

**分类**: `editor_tools/docs`  
**描述**: 查询枚举文档。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `class_name` | `string` | 是 | 类名称 |
| `enum_name` | `string` | 是 | 枚举名称 |

---

## `get_best_practices`

**分类**: `editor_tools/docs`  
**描述**: 获取类的最佳实践建议。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `class_name` | `string` | 是 | 类名称 |