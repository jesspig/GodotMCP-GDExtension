# Undo/Redo 元工具 — 低层设计

> 版本: 1.0 · 2026-06-20  
> 对应迭代: Phase 2 (`feature/capability-catchup`)  
> 状态: 待评审

---

## 1. 模块概述

### 1.1 目标

- 允许 AI 通过 MCP 工具调用 Godot 编辑器的 UndoRedo 系统
- 消除开发者对「AI 搞坏场景无法恢复」的信任危机
- 为 Phase 4 Shadow Scene 的 `apply_changes` / `discard_changes` 提供撤销通道

### 1.2 工具标识

| 属性 | 值 |
|------|-----|
| 工具名 | `undo`, `redo` |
| 源文件 | `extensions/src/built_in/tools/meta/undo.hpp` |
| | `extensions/src/built_in/tools/meta/redo.hpp` |
| 注册方式 | `extensions/src/built_in/tools/register/register_meta.hpp` — 各加一行 `GODOT_MCP_TOOL` |
| `#include` 追加 | `extensions/src/built_in/register_itools.cpp` — 加两行 `#include` |
| 分类 | `meta_tools` |
| `is_meta()` | `true` — 始终在 `tools/list` 可见 |
| `needs_scene()` | `false` — 即使无场景打开也可操作（操作全局 UndoRedo 历史） |
| `supports_undo()` | `false` — 元工具自身不支持撤销撤销 |
| `is_destructive()` | `false` — 不直接修改场景，仅回放历史 |

### 1.3 项目背景

代码库使用 `EditorUndoRedoManager`（Godot 4 多场景 UndoRedo 管理器），非原生 `UndoRedo`。关键差异：

- `EditorUndoRedoManager` 不继承 `UndoRedo`；它管理多个按 history ID 索引的 `UndoRedo*` 实例
- 每个打开的场景有独立的 history ID，通过 `get_object_history_id(Object*)` 获取
- 底层 `UndoRedo*` 通过 `get_history_undo_redo(int32_t history_id)` 获取
- 已有的 `cmd_utils::get_undo_redo()`（`cmd_utils.hpp:64`）返回 `EditorUndoRedoManager*`

---

## 2. 接口设计

### 2.1 类声明 — `UndoTool`

```cpp
// extensions/src/built_in/tools/meta/undo.hpp

#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

namespace godot_mcp {

class UndoTool : public ITool {
public:
    String name() const noexcept override { return "undo"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return "Undo the last editor operation"; }
    String description() const override {
        return "Reverses the most recent action performed in the editor. "
               "Can undo multiple steps in one call. Uses Godot's built-in "
               "UndoRedo system. Returns the action name that was undone (if "
               "available), the number of undone steps, and current history "
               "state (can_undo / can_redo / history_size).";
    }
    bool is_meta() const noexcept override { return true; }

    Dictionary build_input_schema() const override {
        // steps is an optional integer, defaults to 1
        return SchemaBuilder()
            .prop("steps", "integer", "Number of undo steps to perform", 1)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override;
};

} // namespace godot_mcp
```

### 2.2 类声明 — `RedoTool`

```cpp
// extensions/src/built_in/tools/meta/redo.hpp

#pragma once

#include "built_in/tool_base.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/undo_redo.hpp>

namespace godot_mcp {

class RedoTool : public ITool {
public:
    String name() const noexcept override { return "redo"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return "Redo the last undone editor operation"; }
    String description() const override {
        return "Restores the most recently undone action in the editor. "
               "Can redo multiple steps in one call. Uses Godot's built-in "
               "UndoRedo system.";
    }
    bool is_meta() const noexcept override { return true; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("steps", "integer", "Number of redo steps to perform", 1)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override;
};

} // namespace godot_mcp
```

### 2.3 输入/输出 Schema

#### `undo`

**输入：**

```json
{
  "type": "object",
  "properties": {
    "steps": {
      "type": "integer",
      "description": "Number of undo steps to perform",
      "default": 1
    }
  }
}
```

**输出（成功）：**

```json
{
  "success": true,
  "data": {
    "undone_count": 1,
    "action_name": "MCP: Add Node3D",
    "can_undo": true,
    "can_redo": true,
    "history_size": 12
  }
}
```

**输出（空历史）：**

```json
{
  "success": false,
  "error": {
    "code": "NOTHING_TO_UNDO",
    "message": "No operations to undo"
  }
}
```

#### `redo`

输入 schema 与 `undo` 相同。成功/错误字段一致，仅错误码为 `NOTHING_TO_REDO`。

---

## 3. 实现

### 3.1 Undo/Redo 内部机制

代码库的 `EditorUndoRedoManager` 在 godot-cpp 10.0.0-rc1 中**不**直接暴露 `undo()` / `redo()` 方法。需要间接调用：

```
EditorUndoRedoManager*
    │
    ├── get_object_history_id(scene_root) → int32_t history_id
    │
    └── get_history_undo_redo(history_id) → UndoRedo*
                                              │
                                              ├── has_undo() → bool
                                              ├── undo() → bool
                                              ├── redo() → bool
                                              ├── get_current_action_name() → String
                                              ├── get_history_count() → int32_t
                                              └── get_current_action() → int32_t
```

### 3.2 `UndoTool::execute_impl`

```cpp
Dictionary UndoTool::execute_impl(const ToolContext &ctx) {
    int steps = static_cast<int>(ctx.args.get("steps", Variant(1)));

    EditorInterface *ei = EditorInterface::get_singleton();
    if (!ei) {
        return ToolResult::err("NO_EDITOR", "EditorInterface not available");
    }

    EditorUndoRedoManager *ur = ei->get_editor_undo_redo();
    if (!ur) {
        return ToolResult::err("NO_UNDO_REDO", "EditorUndoRedoManager not available");
    }

    // Determine the correct history ID for the current scene.
    // If no scene is open, fall back to GLOBAL_HISTORY (handles
    // project-settings changes, etc.).
    int history_id = static_cast<int>(EditorUndoRedoManager::GLOBAL_HISTORY);
    Node *root = get_root();
    if (root) {
        history_id = ur->get_object_history_id(root);
    }

    UndoRedo *undo_redo = ur->get_history_undo_redo(history_id);
    if (!undo_redo) {
        return ToolResult::err("NO_UNDO_REDO", "UndoRedo instance not available for history");
    }

    if (!undo_redo->has_undo()) {
        return ToolResult::err("NOTHING_TO_UNDO", "No operations to undo");
    }

    int undone_count = 0;
    String last_action_name;

    for (int i = 0; i < steps; i++) {
        if (!undo_redo->has_undo()) break;

        last_action_name = undo_redo->get_current_action_name();
        bool ok = undo_redo->undo();
        if (!ok) break;  // undo() returns false if nothing to undo
        undone_count++;
    }

    Dictionary data;
    data["undone_count"] = undone_count;
    if (!last_action_name.is_empty()) {
        data["action_name"] = last_action_name;
    }
    data["can_undo"] = undo_redo->has_undo();
    data["can_redo"] = undo_redo->has_redo();
    data["history_size"] = undo_redo->get_history_count();

    return ToolResult::ok(data);
}
```

### 3.3 `RedoTool::execute_impl`

与 `UndoTool` 基本相同，变化点：

| `UndoTool` | `RedoTool` |
|-----------|------------|
| `undo_redo->has_undo()` | `undo_redo->has_redo()` |
| `undo_redo->undo()` | `undo_redo->redo()` |
| `NOTHING_TO_UNDO` | `NOTHING_TO_REDO` |

```cpp
Dictionary RedoTool::execute_impl(const ToolContext &ctx) {
    int steps = static_cast<int>(ctx.args.get("steps", Variant(1)));
    // ... same setup as UndoTool ...

    UndoRedo *undo_redo = ur->get_history_undo_redo(history_id);
    if (!undo_redo) {
        return ToolResult::err("NO_UNDO_REDO", "UndoRedo instance not available for history");
    }

    if (!undo_redo->has_redo()) {
        return ToolResult::err("NOTHING_TO_REDO", "No operations to redo");
    }

    int redone_count = 0;
    String last_action_name;

    for (int i = 0; i < steps; i++) {
        if (!undo_redo->has_redo()) break;

        last_action_name = undo_redo->get_current_action_name();
        bool ok = undo_redo->redo();
        if (!ok) break;
        redone_count++;
    }

    Dictionary data;
    data["redone_count"] = redone_count;
    if (!last_action_name.is_empty()) {
        data["action_name"] = last_action_name;
    }
    data["can_undo"] = undo_redo->has_undo();
    data["can_redo"] = undo_redo->has_redo();
    data["history_size"] = undo_redo->get_history_count();

    return ToolResult::ok(data);
}
```

### 3.4 关于 `get_current_action_name()` 的时序

`UndoRedo::get_current_action_name()` 返回当前待定操作（current action）的名称。在调用 `undo()` 前读取该值，得到即将被撤销的操作名；在调用 `redo()` 前读取该值，得到即将被恢复的操作名。这在 Godot 4 的实现中是正确的——`get_current_action()` 返回的是即将 undo/redo 的 action index。

若未来 godot-cpp 版本行为变化，可用 `get_action_name(get_current_action())` 替代。

### 3.5 空场景处理

当 `get_root()` 返回 `nullptr`（无场景打开），`get_object_history_id(nullptr)` 行为未定义。因此统一回退到 `GLOBAL_HISTORY`（值 0）。GLOBAL_HISTORY 记录编辑器层级操作（如 ProjectSettings 变更），同样可撤销。

---

## 4. 注册

### 4.1 X-macro 注册

在 `extensions/src/built_in/tools/register/register_meta.hpp` 添加两行：

```cpp
// extensions/src/built_in/tools/register/register_meta.hpp

GODOT_MCP_TOOL(GetInfoTool, false)
GODOT_MCP_TOOL(GetToolsTool, false)
GODOT_MCP_TOOL(GetCategoriesTool, false)
GODOT_MCP_TOOL(GetToolDetailTool, false)
GODOT_MCP_TOOL(FindToolTool, false)
GODOT_MCP_TOOL(CallToolTool, false)
GODOT_MCP_TOOL(UndoTool, false)    // +1
GODOT_MCP_TOOL(RedoTool, false)    // +1
```

`is_destructive` 为 `false`——undo/redo 仅回放历史，不直接破坏内容。

### 4.2 `#include` 追加

在 `extensions/src/built_in/register_itools.cpp` 添加两行：

```cpp
// extensions/src/built_in/register_itools.cpp

#include "built_in/tools/meta/get_info.hpp"
#include "built_in/tools/meta/get_tools.hpp"
#include "built_in/tools/meta/get_categories.hpp"
#include "built_in/tools/meta/get_tool_detail.hpp"
#include "built_in/tools/meta/find_tool.hpp"
#include "built_in/tools/meta/call_tool.hpp"
#include "built_in/tools/meta/undo.hpp"    // +1
#include "built_in/tools/meta/redo.hpp"    // +1
```

无需修改其他文件（无 codegen，无构建系统变更）。

---

## 5. 错误处理

### 5.1 错误矩阵

| 场景 | `undo` 错误码 | `redo` 错误码 | HTTP 状态码 |
|----------|-------------|-------------|:-----------:|
| 无 EditorInterface 单例 | `NO_EDITOR` | `NO_EDITOR` | 500 |
| 无 EditorUndoRedoManager | `NO_UNDO_REDO` | `NO_UNDO_REDO` | 500 |
| 无对应历史的 UndoRedo | `NO_UNDO_REDO` | `NO_UNDO_REDO` | 500 |
| 无操作可撤销/重做 | `NOTHING_TO_UNDO` | `NOTHING_TO_REDO` | 200 (JSON-RPC error) |
| steps 超出历史数量 | 部分撤销，`undone_count < steps` | 部分重做，`redone_count < steps` | 200 (success) |
| `steps` 为负数或零 | 视为 1（已截断） | 视为 1（已截断） | — |

### 5.2 部分操作语义

当 `steps > available history size`，执行尽可能多的步骤（不报错）。返回值中的 `undone_count` / `redone_count` 反映实际执行数。客户端应检查 `can_undo` / `can_redo` 知晓后续是否可继续执行。

---

## 6. 边界情况

| 场景 | 行为 |
|------|----------|
| 无场景打开 | 使用 `GLOBAL_HISTORY`。仅撤销/重做编辑器级操作。 |
| 多场景打开 | 操作**当前活动/编辑场景**的历史（通过 `ei->get_edited_scene_root()` → `get_object_history_id()`）。 |
| undo 调用间切换场景 | 每次调用针对调用时的活动场景。 |
| 游戏运行时执行 undo | UndoRedo 仍然可访问；游戏状态独立。返回成功但可能不影响游戏画面。 |
| `redo` 后执行 `undo` | 响应中返回 `can_redo: true`（Godot 在 undo 后保留 redo 历史，仅新操作会清除）。 |
| `steps=0` | 视为 1（在 helper 中截断）。 |
| 同一 `_process()` tick 内快速 undo/redo | 在单帧内顺序执行。Godot 的 `undo()`/`redo()` 重入是安全的（不会触发工具重入）。 |
| `has_undo()` 为 false 时调用 `undo` | 立即返回 `NOTHING_TO_UNDO`；不执行部分操作。 |

---

## 7. 集成点

### 7.1 现有 UndoRedo 工具模式

已有大量工具通过 `EditorUndoRedoManager` 提交可撤销操作：

```cpp
// 典型模式（来自 scene_tree/delete_node.hpp 等）
auto *ur = get_undo_redo();
ur->create_action("MCP: Delete node", UndoRedo::MERGE_DISABLE, ctx.root);
ur->add_do_method(parent_node, "remove_child", node);
ur->add_undo_method(parent_node, "add_child", node, true);
ur->add_undo_method(node, "set_owner", ctx.root);
ur->commit_action();
```

这些操作自动被 `undo` / `redo` 工具捕获——无需修改已有工具。

### 7.2 Phase 2 — `supports_undo()` 审计

`ITool` 接口的 `supports_undo()` 虚方法已存在于 `tool_base.hpp:82`，默认返回 `false`。当前未被 `execute()` 前置检查消费。Phase 2 应：

1. **审计所有 `is_destructive() == true` 的工具**，确认它们是否通过 `EditorUndoRedoManager` 提交操作
2. **对于不经过 UndoRedo 的工具**（如 `run_editor_script`、文件写操作），在 `description()` 中注明「此操作不可撤销」
3. **后续考虑**：在 `ToolExecutor` 中根据 `supports_undo()` 自动加入警告提示

### 7.3 Phase 4 — Shadow Scene 兼容性

当 Shadow Scene 系统实现后：

- `apply_changes()` 方法应通过 `EditorUndoRedoManager` 创建一个命名为 `"MCP: Apply scene changes (N diffs)"` 的可撤销操作
- `discard_changes()` 同理创建名为 `"MCP: Discard scene changes"` 的撤销操作
- AI 客户端可以通过 `undo` 来回滚一次完整的 `apply_changes` 操作
- Shadow Scene 的 diff-apply 机制本身由逐条 UndoRedo 操作组成，`undo steps=1` 即可一次性回滚整个 diff 批次（因 `commit_action(true)` 将整个 diff 归为一个 action）

---

## 8. 测试计划

### 8.1 测试文件

创建 `tests/yaml_tests/04_undo_redo.yaml` 和 `tests/yaml_tests/04_redo.yaml`，或并入 `tests/yaml_tests/00_smoke.yaml`。

### 8.2 测试用例

| # | 场景 | 步骤 | 预期 |
|---|----------|-------|----------|
| 1 | 空历史撤销 | `undo` → 无前置操作 | `NOTHING_TO_UNDO` |
| 2 | 空历史重做 | `redo` → 无前置操作 | `NOTHING_TO_REDO` |
| 3 | add_node → undo | `add_node(type=Node3D,name=TestUndo)` → `undo` | 节点被移除；`undone_count: 1`；`action_name` 包含 "MCP: Add" |
| 4 | add_node → undo → 验证已移除 | undo 后执行 `list_scene_items` | 列表中不含 `TestUndo` |
| 5 | add_node → undo → redo | `add_node(Node3D,TestRedo)` → `undo` → `redo` | 节点恢复 |
| 6 | 多步撤销 | `add_node` ×3 → `undo steps=3` | `undone_count: 3` |
| 7 | 部分撤销（steps > 历史） | `add_node` ×1 → `undo steps=5` | `undone_count: 1`，无错误 |
| 8 | can_redo/can_undo 状态 | add_node → undo → 检查响应 | `can_redo: true`, `can_undo: false` |
| 9 | 无场景时的 GLOBAL_HISTORY | 关闭所有场景 → 通过 MCP 修改 ProjectSettings → `undo` | 操作被撤销 |

### 8.3 YAML 测试示例

```yaml
# tests/yaml_tests/04_undo_redo.yaml
name: undo_redo_add_node
steps:
  - tool: add_node
    args: { type: "Node3D", name: "UndoTestNode" }
    expect: { success: true }

  - tool: undo
    args: { steps: 1 }
    expect:
      success: true
      data:
        undone_count: 1

  - tool: list_scene_items
    args: {}
    expect:
      success: true
      # Verify UndoTestNode is no longer present
      data_not_contains: "UndoTestNode"
```

### 8.4 回归测试

所有已有 YAML 测试文件（`tests/yaml_tests/`）必须保持通过，无修改。

---

## 9. 验收标准

1. `undo` 撤销上次编辑器操作（add/delete/modify node），返回 `undone_count` 和 `action_name`
2. `redo` 恢复上次撤销的操作，返回 `redone_count` 和 `action_name`
3. `undo steps=3` 撤销 3 步，`undone_count: 3`
4. 无可撤销操作时 `undo` 返回 `NOTHING_TO_UNDO`；无可重做操作时 `redo` 返回 `NOTHING_TO_REDO`
5. 两工具始终在 `tools/list` 可见（`is_meta=true`）
6. `undo` 和 `redo` 不阻塞主线程，不导致编辑器冻结
7. 所有现有 YAML 测试通过，无回归
8. 无场景打开时也可操作（作用于 GLOBAL_HISTORY）
9. `undo()` / `redo()` 返回值用于判断实际执行（`undo()` returns `false` if nothing to undo）

---

## 10. 未来考虑

| 事项 | 描述 | 优先级 |
|------|-------------|:--------:|
| undo 响应中的操作名 | 已通过 `get_current_action_name()` 实现 | P0 (done) |
| `supports_undo()` 审计 | 检查所有 135 个语义工具的撤销支持 | P1 |
| 撤销历史浏览 | 元工具 `get_undo_history` — 列出最近操作名 | P2 |
| 批量撤销作用域 | `undo steps=-1` = 撤销全部（危险，需确认） | P3 |
| 工具级撤销守卫 | `ToolExecutor` 在 `undo` 待定时拒绝执行？ | P3 |
