# ITool 接口

所有内置 MCP 工具的抽象接口。每个工具继承自 `ITool` 并实现用于元数据和执行逻辑的虚方法。

## 描述

`ITool` 是所有 164 个内置工具的基类。它提供模板方法模式：`execute()` 处理 schema 验证、上下文解析和错误包装，然后委托给 `execute_impl()` 执行工具特定的逻辑。

## 方法

| 返回值 | 签名 | 类型 |
|--------|-----------|------|
| `String` | `name()` | 纯虚 |
| `String` | `brief()` | 纯虚 |
| `String` | `description()` | 虚方法（默认 = `brief()`） |
| `String` | `category()` | 纯虚 |
| `String` | `category_description()` | 虚方法 |
| `Dictionary` | `build_input_schema()` | 虚方法 |
| `bool` | `is_meta()` | 虚方法（默认 = `false`） |
| `bool` | `needs_scene()` | 虚方法（默认 = `false`） |
| `bool` | `needs_node()` | 虚方法（默认 = `false`） |
| `bool` | `supports_undo()` | 虚方法（默认 = `false`） |
| `bool` | `is_destructive()` | 虚方法 |
| `void` | `set_registry(HandlerRegistry*)` | 虚方法 |
| `Dictionary` | `execute(args)` | 非虚（模板方法） |

## 方法说明

### `name()`
* 返回: `String`

唯一的工具标识符（例如 `"add_node"`、`"get_scene_tree"`）。用作 MCP 工具名称。

### `brief()`
* 返回: `String`

显示在工具列表中的一行描述。

### `description()`
* 返回: `String`

详细描述。如果未重写，默认返回 `brief()`。

### `category()`
* 返回: `String`

类别路径（例如 `"editor_tools/scene_tree"`、`"runtime_tools/lifecycle"`）。定义工具在类别树中的位置。

### `build_input_schema()`
* 返回: `Dictionary`

描述工具输入参数的 JSON Schema。格式:
```json
{
  "type": "object",
  "properties": {
    "param_name": { "type": "string", "description": "..." }
  },
  "required": ["param_name"]
}
```

结果缓存在 `schema_cache_` 中。

### `is_meta()`
* 返回: `bool`

如果为 `true`，无论编辑器状态如何，该工具始终在工具列表中可见。

### `needs_scene()`
* 返回: `bool`

如果为 `true`，工具在执行前检查是否有打开的场景。如果没有打开的场景则返回错误。

### `needs_node()`
* 返回: `bool`

如果为 `true`，工具将从参数中解析 `node_path` 或 `path` 参数。如果未找到节点则返回错误。

### `supports_undo()`
* 返回: `bool`

如果为 `true`，工具执行将被包装在 `EditorUndoRedoManager` 中以支持撤销/重做。

### `execute(args: Dictionary)`
* 返回: `Dictionary`

**模板方法** —— 执行流程:
1. 根据 `build_input_schema()` 验证输入参数
2. 解析场景根节点（如果 `needs_scene()`）
3. 解析目标节点（如果 `needs_node()`）
4. 调用 `execute_impl(ctx)`
5. 确保返回的 Dictionary 中存在 `success` 键

### `execute_impl(ctx: ToolContext)`
* 返回: `Dictionary`

**受保护的虚方法** —— 重写此方法以实现工具逻辑。`ToolContext` 提供:
- `root: Node*` —— 当前编辑的场景根节点
- `node: Node*` —— 已解析的目标节点（如果 `needs_node()`）
- `args: Dictionary` —— 已验证的输入参数

## ToolContext

```cpp
struct ToolContext {
    Node *root = nullptr;   // 当前编辑的场景根节点
    Node *node = nullptr;   // 已解析的目标节点
    Dictionary args;         // 已验证的输入参数
};
```

## ToolResult

用于创建标准响应字典的工具类:

```cpp
// 成功
ToolResult::ok(data_dictionary);

// 错误
ToolResult::err("ERROR_CODE", "Human readable message");
```

### 响应格式

```json
// 成功
{ "success": true, "data": { ... } }

// 错误
{ "success": false, "error": { "code": "ERROR_CODE", "message": "..." } }
```