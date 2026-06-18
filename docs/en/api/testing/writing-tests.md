# Writing Tests

## Quick Start

Create a new YAML file in `tests/yaml_tests/`:

```yaml
name: my_feature_test
description: Tests for my new feature
headless: true

pipeline:
  on_failure: fail_fast
  stages:
    - id: main
      steps:
        - id: create_scene
          tool: new_scene
          args:
            root_type: Node2D
          expect:
            status: success
            has_keys: [root_path]

        - id: add_node
          tool: add_node
          args:
            parent_path: /root
            class_name: Sprite2D
            node_name: TestSprite
          expect:
            status: success
            field_checks:
              - key: node_path
                not_empty: true
```

## Best Practices

### 1. Start with a fresh scene

Always create a new scene at the start of your test to avoid interference:

```yaml
- id: setup_scene
  tool: new_scene
  args: { root_type: Node2D }
```

### 2. Use before_all for shared setup

```yaml
pipeline:
  before_all:
    - tool: new_scene
      args: { root_type: Node2D }
```

### 3. Validate tool results thoroughly

```yaml
expect:
  status: success
  has_keys: [node_path, property_value]
  field_checks:
    - key: property_value.position
      value: [100.0, 200.0]
      type: Vector2
      tolerance: 0.001
```

### 4. Test error conditions

```yaml
- id: test_invalid_path
  tool: add_node
  args:
    parent_path: /nonexistent
    class_name: Node2D
  expect:
    status: error
    error_contains: "NODE_NOT_FOUND"
```

### 5. Use disk verification for file operations

```yaml
- id: save_scene
  tool: save_scene
  args:
    path: res://test_output.tscn
  expect:
    status: success
  disk_verify:
    scene:
      path: "res://test_output.tscn"
      nodes:
        - path: "."
          type: "Node2D"
```

### 6. Clean up after yourself

```yaml
pipeline:
  after_all:
    - tool: delete_file
      args: { path: "res://test_output.tscn" }
    - tool: new_scene
      args: { root_type: Node }
```

## Test File Organization

- Name files with two-digit prefixes: `00_meta.yaml`, `01_docs.yaml`, etc.
- Group related tools together in one file
- Mark headless-compatible tests with `headless: true`
- Keep each stage focused on a single feature area

## Running Single Tests

```bash
# Run a specific test file
uv run python main.py test --file 00_meta.yaml

# Run multiple matching files
uv run python main.py test --file "0*"

# Run headless only
uv run python main.py test --headless
```