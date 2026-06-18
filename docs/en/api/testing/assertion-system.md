# Assertion System

The assertion system validates tool execution results against expected values defined in the `expect` block of each test step.

## Expect Block

```yaml
expect:
  status: success           # "success" | "error"
  has_keys: [key1, key2]    # Keys that must exist in result.data
  field_checks:             # Deep value comparisons
    - key: result_field
      value: expected_value
      type: string          # Optional: force type comparison
      tolerance: 0.001      # Float comparison tolerance
      not_empty: true       # Assert value is not empty
  error_contains: "message" # Error string must contain this substring
```

## Assertion Types

### status

```yaml
status: success   # Asserts no "error" key in result
status: error     # Asserts "error" key exists
```

### has_keys

```yaml
has_keys:
  - connection
  - engine
  - plugin
```

Asserts that `result.data` contains all listed keys.

### field_checks

```yaml
field_checks:
  - key: connection.status
    value: "ok"
    not_empty: true
```

Deep field comparison using dot-separated paths. The `key` is traversed into the result data.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `key` | `string` | Yes | Dot-separated path (e.g., `connection.status`) |
| `value` | any | Varies | Expected value (or omit for `not_empty` only) |
| `type` | `string` | No | Force type: string, int, float, bool, Vector2, Vector3, Color |
| `tolerance` | `float` | No | Tolerance for float comparison (default: 0.0001) |
| `not_empty` | `bool` | No | Assert value is not null/empty (default: false) |

### Supported Types for field_checks

| Type | Example Value |
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

Checks if the error string contains the given substring. Matches both the `code: message` concatenation and individual error code/message substrings.

## Disk Verification

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

### Scene Verification

Loads a `.tscn` file and verifies node existence, types, and property values (including nested nodes).

### Project Settings Verification

Loads `project.godot` via `ConfigFile` and checks section/key/value.

### Raw Text Verification

Opens a text file and checks for `contains` / `not_contains` substrings.