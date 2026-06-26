# 影子场景与录制回放工具

> 7 个工具 + 引擎层组件，实现非破坏编辑（Shadow Scene）与操作录制/重放。

## 工具列表

| 工具名 | 文件 | is_destructive | 分类 |
|--------|------|:--------------:|------|
| `stage_scene_change` | `editor_tools/scene/stage_change.hpp` | false | `editor_tools/scene` |
| `preview_change` | `editor_tools/scene/preview_change.hpp` | false | `editor_tools/scene` |
| `apply_changes` | `editor_tools/scene/apply_changes.hpp` | **true** | `editor_tools/scene` |
| `discard_changes` | `editor_tools/scene/discard_changes.hpp` | false | `editor_tools/scene` |
| `diff_scene_states` | `scene_diff/diff_scene_states.hpp` | false | `editor_tools/scene` |
| `record_operations` | `replay/replay_tools.hpp` | false | `editor_tools/scene` |
| `replay_operations` | `replay/replay_tools.hpp` | **true** | `editor_tools/scene` |

## 影子场景（Shadow Scene）

非破坏编辑引擎，位于 `extensions/src/scene_diff/`。详见 `extensions/src/scene_diff/` 源码及 `modules/scene-tree-tools.md`。

### 核心组件

| 类 | 文件 | 职责 |
|---|------|------|
| `SceneShadow` | `scene_diff/scene_shadow.hpp/.cpp` | 快照管理器：capture()、compute_diff()、apply()、rollback() |
| `SceneSnapshot` | `scene_diff/scene_snapshot.hpp/.cpp` | `PackedScene` 快照生成（EditorInterface → PackedScene） |
| `SceneDiff` | `scene_diff/scene_diff.hpp/.cpp` | 差异计算：属性变化 + 节点增删改 + 重亲 |
| `ScenePatcher` | `scene_diff/scene_patcher.hpp/.cpp` | diff → undoable 应用 / rollback 恢复 |
| `DiffSceneStatesTool` | `scene_diff/diff_scene_states.hpp/.cpp` | 独立的 .tscn 文件对比工具（不限 Shadow Scene） |

### 工作流

1. `stage_scene_change` — 捕获当前场景快照到影子缓冲区
2. 用户使用常规场景工具修改场景（add_node、set_node_property 等）
3. `preview_change` — 计算影子快照与当前场景的差异并返回预览
4. `apply_changes` — 将累积变更原子性应用到编辑器（单 undo step）
5. `discard_changes` — 放弃所有暂存变更

通过 `ProjectSettings` 的 `godot_mcp/shadow_scene_enabled` 控制启用状态。

### SceneDiff 结构

```cpp
struct DiffResult {
    Vector<PropertyChange> property_changes;  // 属性变化
    Vector<NodeChange> node_changes;          // 节点变化（ADDED/DELETED/MODIFIED/REPARENTED）
};
```

`ScenePatcher::apply()` 将 diff 通过 `EditorUndoRedoManager` 应用到场景。
`ScenePatcher::rollback()` 将快照 `PackedScene` 直接 instance 恢复。

## 录制与重放

操作录制/重放系统，位于 `extensions/src/replay/`。

| 类 | 文件 | 职责 |
|---|------|------|
| `OperationRecorder` | `replay/operation_recorder.hpp/.cpp` | 录制工具调用序列 → YAML 输出 |
| `OperationReplay` | `replay/operation_replay.hpp/.cpp` | 解析 YAML → 通过 HandlerRegistry 回放 |

### record_operations

- 参数：`action`（string，必填：`"start"` / `"stop"` / `"clear"`）
- `start` 开始录制后续所有工具调用
- `stop` 停止录制并返回已录制操作的 YAML
- 录制数据可直接用于 `replay_operations` 或导出为 YAML 测试

### replay_operations

- 参数：`yaml_content`（string，必填）
- 按 YAML 中定义的步骤顺序回放
- 返回 `ReplayResult`：`success`、`steps_executed`、`steps_total`、`error_message`
- 回放通过 `HandlerRegistry` 直接执行工具，绕过 MCP 协议层

## 相关源文件

| 文件 | 用途 |
|------|------|
| `extensions/src/scene_diff/` | 非破坏编辑引擎 |
| `extensions/src/replay/` | 操作录制与重放 |
| `extensions/src/built_in/tools/editor_tools/scene/` | 4 个 ITool 包装 |
| `extensions/src/scene_diff/diff_scene_states.hpp/.cpp` | diff_scene_states 工具 |
| `extensions/src/replay/replay_tools.hpp/.cpp` | record_operations / replay_operations |
