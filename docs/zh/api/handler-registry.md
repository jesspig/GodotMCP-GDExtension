# HandlerRegistry

**继承**: `Object`

中央调度引擎，管理所有工具（内置和自定义）、搜索索引和类别发现。不可从 GDScript 访问 —— 由 C++ 工具体系内部使用。

## 描述

`HandlerRegistry` 是工具调度系统的核心。它维护 ITool 调度表 `itool_table_`、SDK CommandFn 旁路表、搜索索引、用于搜索排名的频率追踪以及类别树缓存。

## 方法

| 返回值 | 签名 |
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

## 方法说明

### `register_tool(tool: ITool, is_custom: bool)`
* 返回: `void`

注册工具。构建搜索索引并设置工具的注册中心指针（用于需要反向引用的元工具）。

### `execute(name: String, args: Dictionary)`
* 返回: `Dictionary`

按名称查找工具并执行。如果工具支持撤销，则在 `EditorUndoRedoManager` 中包装执行。将调用记录到频率索引中用于搜索排名。

### `get_categories()`
* 返回: `Array`

从所有已注册工具构建递归类别树。使用渐进式披露 —— 仅返回类别，不返回单个工具。结果会被缓存，并在注册/注销时失效。

### `search_tools(query: String, category: String, limit: int)`
* 返回: `Array`

跨工具名称和描述的加权搜索:
- 精确匹配: 权重 1000
- 前缀匹配: 权重 500
- 令牌模糊: 权重 200
- 描述: 权重 50

结果按调用频率排序，然后按权重排序。

### `get_search_suggestions(prefix: String, limit: int)`
* 返回: `Array`

基于前缀的自动补全建议，按调用频率排序。

## 内部流程

```
execute("add_node", args)
  -> record_tool_call("add_node")  // 更新频率
  -> lookup tool in itool_table_
  -> if supports_undo:
      EditorUndoRedoManager::create_action("MCP: add_node")
      tool->execute(args)
      commit_action()
  -> else:
      tool->execute(args)
  -> return result Dictionary
```