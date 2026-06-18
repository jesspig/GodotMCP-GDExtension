# Pipeline Format

YAML pipeline structure for defining test suites.

## Top-Level Structure

```yaml
name: my_test_suite
description: Tests for my feature
headless: true     # Can run in headless mode

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

## Properties

### Pipeline

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `name` | `string` | Yes | - | Suite identifier |
| `description` | `string` | No | - | Suite description |
| `headless` | `bool` | No | `false` | Can run in headless mode |
| `pipeline` | `object` | Yes | - | Pipeline definition |

### Pipeline.on_failure

| Value | Description |
|-------|-------------|
| `fail_fast` | Stop at first failure (default) |
| `stop` | Mark as failed but continue other stages |
| `continue` | Log and continue execution |

### Stage

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | `string` | Yes | Unique stage ID |
| `name` | `string` | No | Display name |
| `steps` | `array` | Yes | Array of step definitions |
| `before_each` | `array` | No | Chain steps before each step |
| `after_each` | `array` | No | Chain steps after each step |
| `on_failure` | `string` | No | Stage-level fail policy |

### Step

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `id` | `string` | Yes | - | Unique step ID within pipeline |
| `tool` | `string` | Yes | - | Tool name to call |
| `description` | `string` | No | - | Step description |
| `args` | `object` | Yes | `{}` | Tool arguments |
| `expect` | `object` | No | - | Assertion block |
| `disk_verify` | `object` | No | - | Disk verification block |
| `depends_on` | `array` | No | - | Step dependency IDs |
| `when` | `object` | No | - | Conditional execution |
| `retry` | `object` | No | - | Retry configuration |
| `allow_failure` | `bool` | No | `false` | Don't fail pipeline on error |
| `on_failure` | `string` | No | - | Step-level fail policy |

### ChainStep (used in before_all, after_all, before_each, after_each)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `tool` | `string` | Yes | Tool name |
| `args` | `object` | Yes | Tool arguments |
| `on_failure` | `string` | No | Fail policy |

## Template Expansion

Step arguments can reference results from previous steps:

```
${steps.<step_id>.result.<path>}
${steps.<step_id>.status}
```

## Conditional Execution (when)

```yaml
when:
  step: previous_step_id
  status: passed  # passed | failed | skipped
```

The step only executes if the referenced step has the specified status.

## Retry

```yaml
retry:
  count: 3
  delay_ms: 1000
```

Retries the step up to 3 times with 1-second delay between attempts.