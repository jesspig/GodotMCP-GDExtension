# Undo/Redo 元工具 — 低层设计（v2）

> 版本: 2.0 · 2026-06-22
> 对应迭代: Phase 2 (`feature/capability-catchup`)
> 状态: 实施中

---

## 版本变更说明

### v1.0 → v2.0 变更原因

原设计（v1.0）计划通过 `EditorUndoRedoManager::get_history_undo_redo()` → 底层 `UndoRedo*` → `undo()`/`redo()` 实现。经过深入调研后放弃该方案，原因：

1. **Godot 引擎明确警告稳定性风险**：`get_history_undo_redo()` 文档写道 *"directly operating on the UndoRedo object might affect editor's stability"*
2. **多次历史 bug**：代码库 git 历史中有 7 次 undo/redo 相关 bug 修复（`4cb871d` 修复 45 个文件、`4967ec6` undo 时序错乱、`bb589dd` action name 捕获时机等）
3. **godot-cpp 10.0.0-rc1 绑定缺失**：`EditorUndoRedoManager` 的 `undo()`/`redo()` 方法未暴露，只能通过底层 `UndoRedo*` 间接调用
4. **MCP 与手动操作无法隔离**：编辑器 UndoRedo 不区分 action 来源，MCP undo 可能撤销用户手动 Ctrl+Z 的操作

### v2.0 方案差异

| 维度 | v1.0（废弃） | v2.0（当前） |
|:----:|:-----------:|:-----------:|
| 撤销机制 | 委托 Godot `UndoRedo::undo()` | MCP 自管理逆向参数栈 |
| 稳定风险 | ⚠️ Godot 警告级别 | ✅ 零，仅普通工具调用 |
| MCP/手动操作隔离 | ❌ 混合 | ✅ 完美独立 |
| 适用范围 | 全部 ~30 个带 undo 的工具 | 属性修改类（~10 个，80% 场景） |
| 步数限制 | 编辑器控制 | ✅ 自定义 16-512 步 |
| 实现复杂度 | ~100 行 | ~150 + 工具埋点 |

---

## 1. 模块概述

### 1.1 目标

- 允许 AI 通过 MCP 工具撤销/重做自己发起的编辑器操作
- 消除开发者对「AI 搞坏场景无法恢复」的信任危机
- **不触碰 Godot EditorUndoRedoManager**，零稳定性风险
- 为 Phase 4 Shadow Scene 的 `apply_changes` / `discard_changes` 提供撤销通道

### 1.2 核心原理：逆向参数栈

每个 MCP 工具在执行时，将其参数和"逆向参数"推入 MCP 管理的独立撤销栈：

```
正向: set_node_property(node, "position", (200,200))
  → push { tool, forward_args, reverse_args: {value: (0,0)} }

undo(1):
  → pop 栈顶，用 reverse_args 调用 set_node_property(node, "position", (0,0))
  → push forward_args 到 redo 栈

redo(1):
  → pop redo 栈，用 forward_args 调用 set_node_property(node, "position", (200,200))
  → push 回 undo 栈
```

对引擎来说，undo/redo 就是一次普通工具调用。没有任何绕过类型系统的操作。

### 1.3 工具标识

| 属性 | `undo` | `redo` | `get_undo_history` |
|:----:|:------:|:------:|:------------------:|
| 源文件 | `meta/undo.hpp` | `meta/redo.hpp` | `meta/get_undo_history.hpp` |
| 分类 | `meta_tools` | `meta_tools` | `meta_tools` |
| `is_meta()` | `true` | `true` | `true` |
| `needs_scene()` | `false` | `false` | `false` |
| `is_destructive()` | `false` | `false` | `false` |

---

## 2. UndoManager 基础设施

### 2.1 核心结构

```cpp
// extensions/src/built_in/cmd_utils/undo_stack.hpp

struct UndoRecord {
    String tool_name;              // 原工具名（如 "set_node_property"）
    Dictionary forward_args;       // 原始参数（用于 redo）
    Dictionary reverse_args;       // 逆向参数（用于 undo，value 为旧值）
    String action_name;            // 人类可读的操作描述
};

class UndoManager {
    std::deque<UndoRecord> undo_;  // 撤销栈
    std::deque<UndoRecord> redo_;  // 重做栈
    int max_steps_;                // 从 ProjectSettings 读取

public:
    UndoManager();

    void push(const UndoRecord &rec);
    bool can_undo() const;
    bool can_redo() const;
    int undo_count() const;
    int redo_count() const;

    UndoRecord pop_undo();  // 调用方负责推入 redo 栈
    UndoRecord pop_redo();  // 调用方负责推入 undo 栈
    void clear_redo();

    void set_max_steps(int n);
    int max_steps() const { return max_steps_; }

    // 只读查询：用于 get_undo_history
    Array peek_undo(int n = 10) const;
    Array peek_redo(int n = 10) const;
};
```

### 2.2 步数限制

| 参数 | 含义 | 默认 | 有效范围 |
|:----:|------|:---:|:--------:|
| `godot_mcp/undo_max_steps` | 撤销栈容量上限 | 64 | 16 ~ 512 |

实现逻辑：

```cpp
void UndoManager::push(const UndoRecord &rec) {
    undo_.push_front(rec);
    while (undo_.size() > max_steps_) {
        undo_.pop_back();    // 丢弃最旧记录
    }
    clear_redo();            // 新操作→清空重做栈
}

void UndoManager::set_max_steps(int n) {
    max_steps_ = std::clamp(n, 16, 512);
    while (undo_.size() > max_steps_) {
        undo_.pop_back();
    }
}
```

ProjectSettings 注册（在 `editor_plugin.cpp` 的 `register_project_settings()` 中）：

```cpp
if (!ps->has_setting("godot_mcp/undo_max_steps")) {
    ps->set_setting("godot_mcp/undo_max_steps", 64);
}
ps->set_initial_value("godot_mcp/undo_max_steps", 64);
ps->set_as_basic("godot_mcp/undo_max_steps", true);

// 运行时读取
int max_steps = static_cast<int>(
    static_cast<int64_t>(ps->get_setting("godot_mcp/undo_max_steps", 64)));
undo_manager_.set_max_steps(max_steps);
```

### 2.3 归属与生命周期

`UndoManager` 由 `McpEditorPlugin` 持有，通过 `HandlerRegistry` 指针传递给需要 recording 的工具：

```
McpEditorPlugin
  └── undo_manager_ (UndoManager)
        └── registry_->set_undo_manager(&undo_manager_)
              └── ITool::set_undo_manager()  ← 每个工具在 register 时得到指针
```

`UndoManager` 生命周期：`McpEditorPlugin::_enter_tree()` 中创建，`_exit_tree()` 中销毁。

---

## 3. 工具接口

### 3.1 undo 工具

```cpp
// extensions/src/built_in/tools/meta/undo.hpp
class UndoTool : public ITool {
public:
    String name() const noexcept override { return "undo"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override {
        return "Undo the last MCP operation by replaying the reverse operation";
    }
    String description() const override {
        return "Reverses the most recent MCP-triggered operation using its "
               "recorded reverse arguments. Does NOT use Godot's built-in "
               "UndoRedo — operates on MCP's independent undo stack. "
               "Returns undo count, action names, and stack state.";
    }
    bool is_meta() const noexcept override { return true; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("steps", "integer",
                  "Number of undo steps to perform (default 1, max stack_size)", 1)
            .build();
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override;
};
```

### 3.2 redo 工具

```cpp
// extensions/src/built_in/tools/meta/redo.hpp
class RedoTool : public ITool {
    // ... 基本同上，仅 brief/description 反映 redo 语义
};
```

### 3.3 get_undo_history 工具（只读）

```cpp
// extensions/src/built_in/tools/meta/get_undo_history.hpp
class GetUndoHistoryTool : public ITool {
public:
    String name() const noexcept override { return "get_undo_history"; }
    String category() const noexcept override { return "meta_tools"; }
    bool is_meta() const noexcept override { return true; }

    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("max_items", "integer",
                  "Max items to return from each stack (default 10)", 10)
            .build();
    }

    Dictionary execute_impl(const ToolContext &ctx) override {
        int n = static_cast<int>(args_int(ctx.args, "max_items", 10));
        Dictionary data;
        data["can_undo"] = undo_manager_->can_undo();
        data["can_redo"] = undo_manager_->can_redo();
        data["undo_stack_size"] = undo_manager_->undo_count();
        data["redo_stack_size"] = undo_manager_->redo_count();
        data["max_steps"] = undo_manager_->max_steps();
        data["undo_items"] = undo_manager_->peek_undo(n);
        data["redo_items"] = undo_manager_->peek_redo(n);
        return ToolResult::ok(data);
    }
};
```

### 3.4 输入/输出 Schema

#### `undo`

**输入：**
```json
{
  "type": "object",
  "properties": {
    "steps": { "type": "integer", "description": "Number of undo steps", "default": 1 }
  }
}
```

**输出（成功）：**
```json
{
  "success": true,
  "data": {
    "undone_count": 1,
    "action_name": "set_node_property: position",
    "can_undo": true,
    "can_redo": true,
    "undo_stack_size": 11,
    "redo_stack_size": 1
  }
}
```

**输出（空历史）：**
```json
{
  "success": false,
  "error": { "code": "NOTHING_TO_UNDO", "message": "No MCP operations to undo" }
}
```

#### `redo`

相同结构，仅错误码为 `NOTHING_TO_REDO`。

---

## 4. 实现

### 4.1 UndoTool::execute_impl

```cpp
Dictionary UndoTool::execute_impl(const ToolContext &ctx) {
    if (!undo_manager_) {
        return ToolResult::err("NO_UNDO_MANAGER", "Undo system not initialized");
    }
    if (!undo_manager_->can_undo()) {
        return ToolResult::err("NOTHING_TO_UNDO", "No MCP operations to undo");
    }

    int steps = std::clamp(
        static_cast<int>(ctx.args.get("steps", Variant(1))),
        1, undo_manager_->undo_count());

    Array action_names;
    int undone_count = 0;

    for (int i = 0; i < steps; i++) {
        if (!undo_manager_->can_undo()) break;

        UndoRecord rec = undo_manager_->pop_undo();
        // 用 reverse_args 重新执行原工具
        Dictionary exec_result = registry_->execute(rec.tool_name, rec.reverse_args);
        if (!exec_result.get("success", false).operator bool()) {
            // 撤销失败：丢弃记录，不推入 redo 栈，继续下一个
            continue;
        }
        // 推入 redo 栈（forward_args 是原始参数）
        UndoRecord redo_rec;
        redo_rec.tool_name = rec.tool_name;
        redo_rec.forward_args = rec.forward_args;
        redo_rec.reverse_args = rec.reverse_args;
        redo_rec.action_name = rec.action_name;
        // （通过 UndoManager 内部直接推入 redo_ 队列）
        undo_manager_->push_redo(redo_rec);

        action_names.append(rec.action_name);
        undone_count++;
    }

    Dictionary data;
    data["undone_count"] = undone_count;
    if (action_names.size() > 0) {
        data["action_name"] = action_names[0];
    }
    data["can_undo"] = undo_manager_->can_undo();
    data["can_redo"] = undo_manager_->can_redo();
    data["undo_stack_size"] = undo_manager_->undo_count();
    data["redo_stack_size"] = undo_manager_->redo_count();
    return ToolResult::ok(data);
}
```

### 4.2 RedoTool::execute_impl

对称逻辑，仅方向相反：

```cpp
UndoRecord rec = undo_manager_->pop_redo();
Dictionary exec_result = registry_->execute(rec.tool_name, rec.forward_args);  // 正向参数
undo_manager_->push_undo(rec);  // 推回 undo 栈
```

### 4.3 工具集成模式：如何推入 UndoRecord

每个支持 undo 的工具在 `execute_impl()` 末尾加 3 行：

```cpp
// 示例：set_node_property.hpp
Dictionary execute_impl(const ToolContext &ctx) override {
    // 1. 捕获旧值
    Variant old_value = node->get(property_name);

    // 2. 执行操作
    node->set(property_name, args["value"]);

    // 3. 推入 undo 记录
    if (undo_manager_) {
        UndoRecord rec;
        rec.tool_name = "set_node_property";
        rec.forward_args = ctx.args;
        rec.reverse_args = ctx.args;
        rec.reverse_args["value"] = old_value;  // 旧值替换新值
        rec.action_name = "set_node_property: " + property_name;
        undo_manager_->push(rec);
    }
    return ToolResult::ok(data);
}
```

Phase 2 覆盖工具清单：

| 工具 | 旧值捕获方式 | 预期记录推入行数 |
|:----:|------------|:--------------:|
| `set_node_property` | `node->get(prop)` | 3 |
| `set_setting` | `ps->get_setting(key)` | 3 |
| `set_keyframe` | `track.get_key_value(idx)` | 4 |
| `set_shader_uniform` | `material->get(shader_param)` | 3 |
| `set_audio_stream` | `player->get("stream")` | 3 |
| `set_theme_override` | `control->get(name)` | 4 |
| `set_game_node_property` | 桥接响应含旧值 | 2 |
| `rename_node` | `node->get("name")` | 2 |
| `set_layout_preset` | 已有 undo 支持 | 2 |
| `set_transition_condition` | `anim_node->get(cond_name)` | 3 |

非覆盖（Phase 3/4 范围）：
- 节点创建/删除（需要 `PackedScene` 快照）
- 文件系统操作（不可逆，设计意图）
- `run_editor_script`（`is_destructive=true`，不可撤销）

---

## 5. 注册

### 5.1 X-macro

```cpp
// register_meta.hpp
GODOT_MCP_TOOL(UndoTool, false)
GODOT_MCP_TOOL(RedoTool, false)
GODOT_MCP_TOOL(GetUndoHistoryTool, false)
```

### 5.2 `#include` 追加

```cpp
// register_itools.cpp
#include "built_in/tools/meta/undo.hpp"
#include "built_in/tools/meta/redo.hpp"
#include "built_in/tools/meta/get_undo_history.hpp"
```

---

## 6. 错误处理

### 6.1 错误矩阵

| 场景 | `undo` | `redo` |
|:----:|:------:|:------:|
| UndoManager 未初始化 | `NO_UNDO_MANAGER` | `NO_UNDO_MANAGER` |
| 无操作可撤销/重做 | `NOTHING_TO_UNDO` | `NOTHING_TO_REDO` |
| steps > 栈大小 | 撤销全部可用记录 | 重做全部可用记录 |
| 逆向操作执行失败 | 丢弃记录，继续下一个 | 丢弃记录，继续下一个 |
| steps 为负数或零 | clamp 到 1 | clamp 到 1 |

### 6.2 逆向操作失败处理

当 `registry_->execute(rec.tool_name, rec.reverse_args)` 返回失败时：
- 该记录被视为**已损坏**（如节点被手动删除导致路径无效）
- 不推入 redo 栈，直接丢弃
- 继续处理下一个记录
- `undone_count` 反映成功执行的逆向操作数

---

## 7. 边界情况

| 场景 | 行为 |
|:----|------|
| 手动在编辑器中修改 | 不进入 MCP 撤销栈。AI undo 可能恢复到旧 MCP 值。此为已知限制。 |
| 多场景操作 | 撤销栈不分场景，按时间顺序。所有 MCP 操作计入同一栈。 |
| 游戏运行时执行 undo | 调用工具时检查桥接是否在线。对运行时桥接工具的 undo 同。 |
| 步数限制动态调整 | `set_max_steps()` 立即裁剪超出部分，从栈底丢弃。 |
| undo 后新操作 | 标准行为：清空 redo 栈。 |
| redo 后 undo | 记录在 undo 栈中，可再次撤销。 |
| `steps=0` | clamp 到 1 |
| 场景切换 | 不影响撤销栈。记录仅按工具调用顺序排列。 |

### 手动修改导致旧值漂移

```
时间线:
  1. MCP set_node_property(pos, 100→200)  → push {old:100, new:200}
  2. 手动在编辑器将 pos 改为 300
  3. MCP undo                            → set_node_property(pos, 100)
                                        → 编辑器 pos 变为 100（而非 300）
```

此为用户手动编辑与 MCP 撤销栈不一致导致的预期行为。`get_undo_history` 工具可帮助 AI 在 undo 前检查当前状态。

---

## 8. 集成点

### 8.1 与现有 EditorUndoRedoManager 的关系

- 已有工具通过 `EditorUndoRedoManager` 提交的操作**不受影响**
- 编辑器自身 Ctrl+Z/Ctrl+Shift+Z 仍然可用
- MCP undo/redo **不与编辑器 UndoRedo 交互**
- 两种撤销路径平行存在，互不干扰

### 8.2 Phase 4 — Shadow Scene 兼容性

当 Shadow Scene 系统实现后：
- `apply_changes()` / `discard_changes()` 暴露为 MCP 工具
- 它们也会推入 MCP undo 栈
- 一个 `apply_changes()` 操作 = 1 条 undo 记录

---

## 9. 测试计划

### 9.1 测试文件

- `tests/yaml_tests/04_undo_redo.yaml`
- `tests/yaml_tests/05_get_undo_history.yaml`

### 9.2 测试用例

| # | 场景 | 步骤 | 预期 |
|:-:|------|------|------|
| 1 | 空历史撤销 | `undo` → 无前置操作 | `NOTHING_TO_UNDO` |
| 2 | 空历史重做 | `redo` → 无前置操作 | `NOTHING_TO_REDO` |
| 3 | 属性修改 → undo | `set_node_property(x=200)` → `undo` | x 恢复为旧值；`undone_count:1` |
| 4 | undo → redo | 上一步后 `redo` | x 重新变为 200 |
| 5 | 多步撤销 | 3 次 set 调用 → `undo steps=3` | `undone_count: 3` |
| 6 | 部分撤销 | 3 次操作 → `undo steps=5` | `undone_count: 3`，无错误 |
| 7 | 跨工具 undo | `set_setting` → `undo` | 设置恢复旧值 |
| 8 | get_undo_history | 操作后调用 | 返回 undo/redo 栈内容 |
| 9 | 步数限制 | push 超过 max_steps | 栈大小不超过 max_steps |
| 10 | 重做栈清空 | undo → 新操作 | redo 栈为空 |

### 9.3 回归测试

所有已有 YAML 测试文件必须保持通过，无修改。

---

## 10. 验收标准

1. `set_node_property(position=200)` → `undo` → position 恢复为旧值
2. `set_node_property` → `undo` → `redo` → position 重新变为 200
3. `undo steps=3` 撤销 3 步，`undone_count: 3`
4. 空历史时 `undo` 返回 `NOTHING_TO_UNDO`；空重做栈时 `redo` 返回 `NOTHING_TO_REDO`
5. 3 个工具始终在 `tools/list` 可见（`is_meta=true`）
6. `undo`/`redo` 不触碰 `EditorUndoRedoManager`
7. 所有现有 YAML 测试通过，无回归
8. `get_undo_history` 可读查看撤销/重做栈信息
9. `godot_mcp/undo_max_steps` 默认 64，可配置范围 16-512

---

## 11. 局限性与未来考虑

| 事项 | 描述 | 优先级 |
|:----|------|:------:|
| 节点创建/删除 undo | 需要 `PackedScene` 快照 + 文件 I/O | Phase 3 |
| 文件操作 undo | 文件系统变更不可撤销（设计意图） | 永不 |
| 旧值漂移检测 | undo 前自动 `get_node_property` 检查当前值与记录旧值是否一致 | P2 |
| `steps=-1` 全撤销 | 危险操作，需确认保护机制 | P3 |
| undo 失败重试 | 逆向操作首次失败后是否重试？ | P3 |
