# 工作区与调试器工具

13 个用于控制编辑器工作区、调试器、控制台和断点的工具。

## 视口捕获

### `capture_viewport`

**分类**: `editor_tools/workspace`  
**描述**: 将编辑器视口捕获为 PNG 图片。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `path` | `string` | 否 | temp | 截图保存路径 |

#### 返回值

```json
{ "success": true, "data": { "path": "res://.godot/mcp/screenshot_1234.png" } }
```

### `capture_game_viewport`

**分类**: `editor_tools/workspace`  
**描述**: 运行时捕获游戏视口。  
**破坏性**: 否 | **撤销**: 否

---

## 控制台

### `clear_console`

**分类**: `editor_tools/workspace`  
**描述**: 清除编辑器控制台输出。  
**破坏性**: 否 | **撤销**: 否

### `get_console_output`

**分类**: `editor_tools/workspace`  
**描述**: 获取控制台输出（可筛选）。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `filter` | `string` | 否 | - | 筛选条件："error", "warning", "all" |
| `max_lines` | `int` | 否 | 100 | 返回的最大行数 |

---

## 调试器控制

### `get_debugger_state`

**分类**: `editor_tools/workspace`  
**描述**: 获取调试器状态和错误列表。  
**破坏性**: 否 | **撤销**: 否

### `get_performance_monitors`

**分类**: `editor_tools/workspace`  
**描述**: 获取性能监视器数据。  
**破坏性**: 否 | **撤销**: 否

#### 返回值

```json
{
  "success": true,
  "data": {
    "fps": 60,
    "memory": { "static": 52428800, "dynamic": 10485760 },
    "objects": { "total": 1500, "nodes": 200 },
    "physics": { "fps": 60 },
    "render": { "draw_calls": 100 }
  }
}
```

### `get_stack_trace`

**分类**: `editor_tools/workspace`  
**描述**: 获取当前堆栈跟踪。  
**破坏性**: 否 | **撤销**: 否

### `get_locals`

**分类**: `editor_tools/workspace`  
**描述**: 获取断点处的局部变量。  
**破坏性**: 否 | **撤销**: 否

### `debugger_control`

**分类**: `editor_tools/workspace`  
**描述**: 控制调试器。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `action` | `string` | 是 | 动作："break", "continue", "step_into", "step_over", "step_out" |

---

## 断点

### `list_breakpoints`

**分类**: `editor_tools/workspace`  
**描述**: 列出所有断点。  
**破坏性**: 否 | **撤销**: 否

### `set_breakpoint`

**分类**: `editor_tools/workspace`  
**描述**: 设置断点。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 脚本路径 |
| `line` | `int` | 是 | 行号 |

### `remove_breakpoint`

**分类**: `editor_tools/workspace`  
**描述**: 移除断点。  
**破坏性**: 否 | **撤销**: 否

---

## 工作区切换

### `set_workspace`

**分类**: `editor_tools/workspace`  
**描述**: 切换编辑器工作区。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `workspace` | `string` | 是 | 工作区："2D", "3D", "Script", "AssetLib" |