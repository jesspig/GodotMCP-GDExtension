# 断言系统

断言系统根据每个测试步骤中 `expect` 块定义的预期值验证工具执行结果。

## Expect 块

```yaml
expect:
  status: success           # "success" | "error"
  has_keys: [key1, key2]    # 必须存在于 result.data 中的键
  field_checks:             # 深度值比较
    - key: result_field
      value: expected_value
      type: string          # 可选：强制类型比较
      tolerance: 0.001      # 浮点数比较容差
      not_empty: true       # 断言值不为空
  error_contains: "message" # 错误字符串必须包含此子串
```

## 断言类型

### status

```yaml
status: success   # 断言结果中没有 "error" 键
status: error     # 断言结果中存在 "error" 键
```

### has_keys

```yaml
has_keys:
  - connection
  - engine
  - plugin
```

断言 `result.data` 包含列出的所有键。

### field_checks

```yaml
field_checks:
  - key: connection.status
    value: "ok"
    not_empty: true
```

使用点分隔路径进行深度字段比较。`key` 被遍历到结果数据中。

| 字段 | 类型 | 必需 | 描述 |
|-------|------|----------|-------------|
| `key` | `string` | 是 | 点分隔路径（例如 `connection.status`） |
| `value` | any | 视情况 | 预期值（如仅需要 `not_empty` 则可省略） |
| `type` | `string` | 否 | 强制类型: string, int, float, bool, Vector2, Vector3, Color |
| `tolerance` | `float` | 否 | 浮点数比较的容差（默认: 0.0001） |
| `not_empty` | `bool` | 否 | 断言值不为 null/空（默认: false） |

### field_checks 支持的类型

| 类型 | 示例值 |
|------|---------------|
| `string` | `"hello"` |
| `int` | `42` |
| `float` | `3.14` |
| `bool` | `true` |
| `Vector2` | `[1.0, 2.0]` |
| `Vector2i` | `[1, 2]` |
| `Vector3` | `[1.0, 2.0, 3.0]` |
| `Vector3i` | `[1, 2, 3]` |
| `Vector4` | `[1.0, 2.0, 3.0, 4.0]` |
| `Color` | `[1.0, 0.0, 0.0, 1.0]` |

### error_contains

```yaml
error_contains: "TOOL_NOT_FOUND: nonexistent"
```

检查错误字符串是否包含给定的子串。同时匹配 `code: message` 拼接格式和单独的错误码/消息子串。

## 磁盘验证

```yaml
disk_verify:
  scene:
    path: "res://test_scene.tscn"
    nodes:
      - path: "Player"
        type: "CharacterBody2D"
        properties:
          position: { x: 100, y: 200 }
  project_settings:
    section: "application"
    key: "config/name"
    value: "My Game"
  raw_text:
    path: "res://test.gd"
    contains: "extends Node"
    not_contains: "print(\"debug\")"
```

### 场景验证

加载 `.tscn` 文件并验证节点是否存在、类型和属性值（包括嵌套节点）。

### 项目设置验证

通过 `ConfigFile` 加载 `project.godot` 并检查 section/key/value。

### 原始文本验证

打开文本文件并检查 `contains` / `not_contains` 子串。