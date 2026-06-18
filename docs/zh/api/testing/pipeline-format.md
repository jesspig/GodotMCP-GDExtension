# 管道格式

用于定义测试套件的 YAML 管道结构。

## 顶层结构

```yaml
name: my_test_suite
description: Tests for my feature
headless: true     # 可在 headless 模式下运行

pipeline:
  on_failure: fail_fast  # fail_fast | stop | continue

  before_all:
    - tool: setup_tool
      args: { key: value }

  stages:
    - id: main
      name: Main Tests
      before_each:
        - tool: prepare
          args: {}
      steps:
        - id: step_1
          tool: my_tool
          description: What this step tests
          args:
            param1: value1
          expect:
            status: success
            has_keys: [result_key]
            field_checks:
              - key: result_key.field
                value: expected_value

  after_all:
    - tool: cleanup_tool
      args: {}
```

## 属性

### Pipeline

| 字段 | 类型 | 必需 | 默认值 | 描述 |
|-------|------|----------|---------|-------------|
| `name` | `string` | 是 | - | 套件标识符 |
| `description` | `string` | 否 | - | 套件描述 |
| `headless` | `bool` | 否 | `false` | 可在 headless 模式下运行 |
| `pipeline` | `object` | 是 | - | 管道定义 |

### Pipeline.on_failure

| 值 | 描述 |
|-------|-------------|
| `fail_fast` | 遇到第一个失败时停止（默认） |
| `stop` | 标记为失败但继续其他阶段 |
| `continue` | 记录日志并继续执行 |

### Stage

| 字段 | 类型 | 必需 | 描述 |
|-------|------|----------|-------------|
| `id` | `string` | 是 | 唯一的阶段 ID |
| `name` | `string` | 否 | 显示名称 |
| `steps` | `array` | 是 | 步骤定义数组 |
| `before_each` | `array` | 否 | 在每个步骤前执行的链式步骤 |
| `after_each` | `array` | 否 | 在每个步骤后执行的链式步骤 |
| `on_failure` | `string` | 否 | 阶段级别的失败策略 |

### Step

| 字段 | 类型 | 必需 | 默认值 | 描述 |
|-------|------|----------|---------|-------------|
| `id` | `string` | 是 | - | 管道内唯一的步骤 ID |
| `tool` | `string` | 是 | - | 要调用的工具名称 |
| `description` | `string` | 否 | - | 步骤描述 |
| `args` | `object` | 是 | `{}` | 工具参数 |
| `expect` | `object` | 否 | - | 断言块 |
| `disk_verify` | `object` | 否 | - | 磁盘验证块 |
| `depends_on` | `array` | 否 | - | 步骤依赖 ID |
| `when` | `object` | 否 | - | 条件执行 |
| `retry` | `object` | 否 | - | 重试配置 |
| `allow_failure` | `bool` | 否 | `false` | 出错时不使管道失败 |
| `on_failure` | `string` | 否 | - | 步骤级别的失败策略 |

### ChainStep（用于 before_all、after_all、before_each、after_each）

| 字段 | 类型 | 必需 | 描述 |
|-------|------|----------|-------------|
| `tool` | `string` | 是 | 工具名称 |
| `args` | `object` | 是 | 工具参数 |
| `on_failure` | `string` | 否 | 失败策略 |

## 模板展开

步骤参数可以引用先前步骤的结果:

```
${steps.<step_id>.result.<path>}
${steps.<step_id>.status}
```

## 条件执行（when）

```yaml
when:
  step: previous_step_id
  status: passed  # passed | failed | skipped
```

仅当引用的步骤具有指定状态时，该步骤才执行。

## 重试

```yaml
retry:
  count: 3
  delay_ms: 1000
```

最多重试步骤 3 次，每次尝试之间延迟 1 秒。