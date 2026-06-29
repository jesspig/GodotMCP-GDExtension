# 脚本工具

13 个用于读写和管理 GDScript 及 C# 脚本的工具。

## GDScript 工具（8 个）

### `read_gd_script`

**分类**: `editor_tools/scripts`  
**描述**: 读取 GDScript 文件。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 脚本文件路径 |

#### 返回值

```json
{ "success": true, "data": { "content": "extends Node\n\nfunc _ready():\n    pass\n", "path": "res://player.gd" } }
```

---

### `write_gd_script`

**分类**: `editor_tools/scripts`  
**描述**: 写入或创建 GDScript 文件。  
**破坏性**: 是 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 脚本文件路径 |
| `content` | `string` | 是 | 脚本内容 |

---

### `patch_gd_script`

**分类**: `editor_tools/scripts`  
**描述**: 修补 GDScript 文件（查找并替换）。  
**破坏性**: 是 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 脚本文件路径 |
| `old_string` | `string` | 是 | 要替换的文本 |
| `new_string` | `string` | 是 | 替换后的文本 |

---

### `validate_gd_script`

**分类**: `editor_tools/scripts`  
**描述**: 通过 LSP 验证 GDScript 语法。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 脚本文件路径 |

#### 返回值

```json
{ "success": true, "data": { "valid": true } }
```

或包含错误信息：

```json
{ "success": true, "data": { "valid": false, "errors": [{"line": 5, "message": "Unexpected token"}] } }
```

---

### `list_gd_scripts`

**分类**: `editor_tools/scripts`  
**描述**: 列出项目中所有 GDScript 文件。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 否 | 搜索目录（默认: res://） |

---

### `grep_scripts`

**分类**: `editor_tools/scripts`  
**描述**: 在脚本中搜索文本。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `pattern` | `string` | 是 | 搜索模式 |
| `include` | `string` | 否 | 文件模式筛选 |

---

### `glob_scripts`

**分类**: `editor_tools/scripts`  
**描述**: 按模式搜索脚本文件。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `pattern` | `string` | 是 | Glob 模式 |
| `path` | `string` | 否 | 搜索根目录 |

---

### `run_editor_script`

**分类**: `editor_tools/scripts`  
**描述**: 在编辑器上下文中执行 EditorScript (.gd) 文件。  
**破坏性**: 是 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 脚本文件路径（必须继承 EditorScript） |
| `timeout_ms` | `integer` | 否 | 最大执行时间（毫秒，默认 5000，最大 30000） |

#### 返回值

```json
{ "success": true, "data": { "path": "res://my_script.gd", "execution_time_ms": 1234, "stdout": "", "timed_out": false } }
```

---

## C# 脚本工具（5 个）

### `read_csharp_script`

**分类**: `editor_tools/scripts`  
**描述**: 读取 C# 脚本文件。  
**破坏性**: 否 | **撤销**: 否

### `write_csharp_script`

**分类**: `editor_tools/scripts`  
**描述**: 写入或创建 C# 脚本文件。  
**破坏性**: 是 | **撤销**: 否

### `patch_csharp_script`

**分类**: `editor_tools/scripts`  
**描述**: 修补 C# 脚本文件（查找并替换）。  
**破坏性**: 是 | **撤销**: 否

### `validate_csharp_script`

**分类**: `editor_tools/scripts`  
**描述**: 通过编译验证 C# 脚本语法。  
**破坏性**: 否 | **撤销**: 否

### `list_csharp_scripts`

**分类**: `editor_tools/scripts`  
**描述**: 列出所有 C# 脚本文件。  
**破坏性**: 否 | **撤销**: 否

> 参数与其 GDScript 对应项相同。