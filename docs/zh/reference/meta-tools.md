# 元工具

7 个用于工具发现、内省和配置的工具。

## `get_info`

**分类**: `meta_tools`  
**描述**: 获取编辑器连接状态和运行时环境信息。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

无。

### 返回值

```json
{
  "success": true,
  "data": {
    "connection": { "status": "ok" },
    "engine": { "version": "4.6", "major": 4, "minor": 6 },
    "plugin": { "version": "0.2.2-dev1", "mode": "editor" },
    "project": { "name": "My Game", "path": "res://" },
    "editor": { "scene_open": true, "node_count": 5 }
  }
}
```

### 示例

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_info","arguments":{}}}'
```

---

## `get_tools`

**分类**: `meta_tools`  
**描述**: 列出所有已注册的工具。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `category` | `string` | 否 | 按分类路径筛选 |
| `query` | `string` | 否 | 搜索查询筛选 |

### 返回值

工具元数据数组（name, category, brief, input_schema）。

### 示例

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_tools","arguments":{"category":"editor_tools/scene_tree"}}}'
```

---

## `get_categories`

**分类**: `meta_tools`  
**描述**: 列出工具分类树。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

无。

### 返回值

嵌套的分类树结构。

---

## `get_tool_detail`

**分类**: `meta_tools`  
**描述**: 获取指定工具的完整元数据。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `name` | `string` | 是 | 工具名称 |

### 返回值

包含 input_schema 和描述信息的完整工具元数据。

---

## `find_tool`

**分类**: `meta_tools`  
**描述**: 全文搜索工具。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `query` | `string` | 是 | 搜索查询 |
| `category` | `string` | 否 | 按分类筛选 |
| `limit` | `int` | 否 | 最大结果数（默认: 20） |

---

## `call_tool`

**分类**: `meta_tools`  
**描述**: 按名称调用任意工具。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `name` | `string` | 是 | 要调用的工具名称 |
| `arguments` | `object` | 是 | 工具参数 |

---

## `generate_client_config`

**分类**: `meta_tools`  
**描述**: 生成 MCP 客户端配置文件。  
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `client_type` | `string` | 否 | 客户端类型（opencode, cursor, cline 等） |
| `output_path` | `string` | 否 | 自定义输出路径 |