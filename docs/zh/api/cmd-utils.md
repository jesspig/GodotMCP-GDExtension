# cmd_utils —— 共享工具模板

7 个工具头文件，提供用于构建 ITool 实现的可复用模式和模板。

## SchemaBuilder

**文件**: `cmd_utils/schema_builder.hpp`

用于构建 JSON Schema 字典的流畅构建器。

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

### 方法

| 返回值 | 方法 | 描述 |
|--------|--------|-------------|
| `SchemaBuilder&` | `param(name, type, description)` | 添加参数 |
| `SchemaBuilder&` | `required()` | 将上一个参数标记为必需 |
| `Dictionary` | `build()` | 完成并返回 schema |

### 支持的类型

| 常量 | Godot 类型 |
|----------|------------|
| `TYPE_STRING` | `String` |
| `TYPE_INT` | `int` |
| `TYPE_FLOAT` | `float` |
| `TYPE_BOOL` | `bool` |
| `TYPE_ARRAY` | `Array` |
| `TYPE_OBJECT` | `Dictionary` |

---

## DispatchMap

**文件**: `cmd_utils/dispatch_map.hpp`

编译期字符串到字符串的查找表。替换用于 schema 推断调度的 `if/else if` 链。

```cpp
static const DispatchMap<3> NODE_TYPE_MAP = {{
    {"2d", "Node2D"},
    {"3d", "Node3D"},
    {"ui", "Control"},
}};
```

---

## UndoHelpers

**文件**: `cmd_utils/undo_helpers.hpp`

用于工具实现的统一撤销/重做模式。

```cpp
undoable_set(node, "position", new_value, old_value);
```

自动在 `EditorUndoRedoManager` 中记录操作前后的值。

---

## ArgsGetTyped

**文件**: `cmd_utils/args_get_typed.hpp`

带默认值的类型安全参数提取。

```cpp
String parent_path = args_get<String>(args, "parent_path");
int index = args_get<int>(args, "index", -1);
bool enabled = args_get<bool>(args, "enabled");
```

类型不匹配时抛出 `Dictionary` 错误（符合 ToolResult 格式）。

---

## ErrorCodes

**文件**: `cmd_utils/error_codes.hpp`

所有工具使用的标准错误码常量。

| 常量 | 值 | 描述 |
|----------|-------|-------------|
| `ERR_TOOL_NOT_FOUND` | `"TOOL_NOT_FOUND"` | 未找到工具名称 |
| `ERR_SCENE_REQUIRED` | `"SCENE_REQUIRED"` | 没有打开的场景 |
| `ERR_NODE_NOT_FOUND` | `"NODE_NOT_FOUND"` | 未找到节点路径 |
| `ERR_INVALID_PARAM` | `"INVALID_PARAM"` | 无效参数 |
| `ERR_TIMEOUT` | `"TIMEOUT"` | 操作超时 |
| `ERR_INTERNAL` | `"INTERNAL_ERROR"` | 内部错误 |

---

## MemdeleteGuard

**文件**: `cmd_utils/memdelete_guard.hpp`

用于安全 `memdelete` 的 RAII 守卫。确保即使发生异常也能释放 Godot 对象。

```cpp
auto new_node = memnew(Node2D);
MemdeleteGuard guard(new_node);
// 如果这里发生任何异常，new_node 会被安全释放
guard.release(); // 将所有权转移给场景树
```

---

## TrackedSettings

**文件**: `cmd_utils/tracked_settings.hpp`

追踪项目设置修改用于批量恢复。

```cpp
TrackedSettings settings;
settings.track("application/config/name");
settings.set("application/config/name", "New Name");
// ... 稍后 ...
settings.restore(); // 恢复所有追踪的更改
```