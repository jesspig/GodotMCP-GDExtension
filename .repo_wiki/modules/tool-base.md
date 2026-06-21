# ITool 基类体系

> 所有 MCP 工具的统一接口层，位于 `extensions/src/built_in/tool_base.hpp` 和 `tool_base.cpp`。包含三个核心类型：`ToolResult`（统一返回信封）、`ToolContext`（前置注入上下文）、`ITool`（抽象基类 + 模板方法）。

## ToolResult — 统一返回信封

静态工厂方法全部在 `tool_base.hpp` 声明，`tool_base.cpp` 实现。

### 成功信封

```json
{"success": true, "data": {...}}
```

| 工厂方法 | 额外字段 |
|----------|---------|
| `ToolResult::ok(data)` | 仅 `data`（为空时省略） |

### 失败信封

```json
{"success": false, "error": {"code": "...", "message": "..."}}
```

| 工厂方法 | 额外字段 |
|----------|---------|
| `ToolResult::err(code, message)` | 标准错误 |

### 内部结构

```mermaid
classDiagram
    class ToolResult {
        +static ok(Dictionary data) Dictionary
        +static err(String code, String message) Dictionary
    }
    class ToolContext {
        +Node* root
        +Node* node
        +Dictionary args
        +Variant jsonrpc_id
    }
    class ITool {
        <<abstract>>
        +String name()*
        +String brief()*
        +String description()
        +Dictionary input_schema()
        +String category()*
        +String category_description()
        +bool is_meta()
        +bool needs_scene()
        +bool needs_node()
        +bool supports_undo()
        +bool is_destructive()
        +void set_is_destructive(bool)
        +void set_registry(HandlerRegistry*)
        +Dictionary execute(Dictionary args)
        #Dictionary build_input_schema()
        #Dictionary execute_impl(const ToolContext &ctx)*
        #bool is_destructive_
    }
    ToolContext --> ITool : execute() 注入
```

## ToolContext — 前置注入上下文

位于 `tool_base.hpp`：

```cpp
struct ToolContext {
    Node *root = nullptr;     // 场景根节点（needs_scene=true 时保证非空）
    Node *node = nullptr;     // 目标节点（needs_node=true 时保证非空）
    Dictionary args;          // 原始参数字典
    Variant jsonrpc_id;       // JSON-RPC 请求 ID（用于 SSE 等异步推送定位）
};
```

- `root`：仅当 `needs_scene()` 返回 `true` 时由 `execute()` 自动注入
- `node`：仅当 `needs_node()` 返回 `true` 时由 `resolve_node()` 自动注入
- `args`：客户端原始参数字典，按需通过 `cmd_utils` 的 `args_string/int/float/bool` 安全读取
- `jsonrpc_id`：客户端请求 ID，由 `HandlerRegistry::execute()` 在 `args` 之外额外注入

## ITool 类图

### 元数据（纯虚函数）

| 方法 | 返回 | 说明 |
|------|------|------|
| `name()` | `String` | 工具注册名，全局唯一 |
| `brief()` | `String` | 简短描述（工具列表用） |
| `build_input_schema()` | `Dictionary` | JSON Schema 格式参数定义 |
| `category()` | `String` | 分类路径（如 `editor_tools/scene_tree`） |

### 元数据（虚函数 + 默认值）

| 方法 | 默认值 | 说明 |
|------|--------|------|
| `description()` | `{ return brief(); }` | 完整描述（含用法说明），默认等同 `brief()` |
| `input_schema()` | 非虚，缓存 `build_input_schema()` | JSON Schema 参数定义，子类覆写 `build_input_schema()` |

### 分类（虚函数 + 默认值）

| 方法 | 默认值 | 说明 |
|------|--------|------|
| `category_description()` | `{}` | 分类说明文本 |
| `is_meta()` | `false` | 渐进式披露：meta 工具始终在 `tools/list` 可见 |
| `needs_scene()` | `false` | 是否需要已打开的场景（触发 `ctx.root` 注入） |
| `needs_node()` | `false` | 是否需要节点路径（触发 `ctx.node` 注入） |
| `supports_undo()` | `false` | 是否支持撤销（框架不处理，子类自行调 `undoable_set`） |
| `is_destructive()` | `is_destructive_` | 标记为可能破坏性操作 |
| `set_registry()` | `{}` | 依赖注入 HandlerRegistry 指针 |

### `is_destructive_` 标记

通过 `GODOT_MCP_TOOL(cls, is_destructive_val)` 宏的第 2 个参数设置。用于：
- AI 客户端在调用前提示用户确认
- 元工具 `call_tool` 检查 `is_destructive()` 并追加确认步骤

## `execute()` 模板方法流程

```mermaid
flowchart TD
    A["ITool::execute(args)"] --> B["Input Schema Validation"]
    B --> C{"needs_scene()?"}
    C -- "是" --> D["get_root_or_error(out_err)"]
    D --> E{"root == nullptr?"}
    E -- "是" --> F["return err(INVALID_SCENE_ROOT)"]
    E -- "否" --> G{"needs_node()?"}
    C -- "否" --> G
    G -- "是" --> H["resolve_node(root, node_path)"]
    H --> I{"node == nullptr?"}
    I -- "是" --> J["return err(NODE_NOT_FOUND)"]
    I -- "否" --> K["execute_impl(ctx)"]
    G -- "否" --> K
    K --> L{"result.has('success')?"}
    L -- "否" --> M["自动包裹 success=true"]
    L -- "是" --> N["return result"]
    M --> N
```

### 步骤 1: Input Schema Validation

1. **Required 参数检查**：遍历 `schema["required"]` 数组，缺失返回 `MISSING_REQUIRED_PARAM`
2. **类型检查**：对 `schema["properties"][name]["type"]` 映射到 `Variant::Type`

### 类型验证映射表

| Schema `type` | 允许的 `Variant::Type` |
|---------------|----------------------|
| `"string"` | `STRING` |
| `"integer"` | `INT`, `FLOAT` |
| `"number"` | `FLOAT`, `INT` |
| `"boolean"` | `BOOL` |
| `"array"` | `ARRAY` |
| `"object"` | `DICTIONARY` |

`FLOAT` 可通过 integer 类型检查，这是 `cmd_utils_json.json_to_variant` 中 `{x,y}` 等结构被解析为 `Vector2` 后兼容字符串/数值混合参数的必要设计。

### 步骤 2: 场景根节点解析

`needs_scene()` → `get_root_or_error()` → 失败返回 `err("INVALID_SCENE_ROOT", ...)`

### 步骤 3: 节点路径解析

`needs_node()` → 尝试 `args["node_path"]` → 后备 `args["path"]` → 空则返回 `err("MISSING_PARAM")` → `resolve_node(root, node_path)` → 失败返回 `err("NODE_NOT_FOUND")`

### 步骤 4: 执行业务逻辑

`execute_impl(ctx)` — 子类实现实际功能。

### 步骤 5: 安全包裹

检查返回字典是否有 `"success"` 键；若无则自动包裹为 `{"success": true, "data": result}`。

## 子类实现契约

```cpp
class MyTool : public ITool {
public:
    String name() const override { return "my_tool"; }
    String category() const override { return "editor_tools/my_category"; }
    String brief() const override { return "短描述"; }
    bool needs_scene() const override { return true; }

protected:
    Dictionary build_input_schema() const override {
        Dictionary schema;
        // 定义参数属性
        return schema;
    }
    Dictionary execute_impl(const ToolContext &ctx) override {
        // 业务逻辑 — ctx.root 保证非空
        return ToolResult::ok(some_data);
    }
};
```

## 注册方式

通过 `GODOT_MCP_TOOL(MyTool, false)` X-macro 注册（`register/register_*.hpp`），不需要手动实例化。详见 [x-macro-registration.md](x-macro-registration.md)。
