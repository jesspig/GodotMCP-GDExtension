# 编写测试

## 快速开始

在 `tests/yaml_tests/` 中创建新的 YAML 文件:

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

## 最佳实践

### 1. 从新场景开始

始终在测试开始时创建新场景以避免干扰:

```yaml
- id: setup_scene
  tool: new_scene
  args: { root_type: Node2D }
```

### 2. 使用 before_all 进行共享设置

```yaml
pipeline:
  before_all:
    - tool: new_scene
      args: { root_type: Node2D }
```

### 3. 彻底验证工具结果

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

### 4. 测试错误条件

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

### 5. 使用磁盘验证检查文件操作

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

### 6. 执行完毕后清理

```yaml
pipeline:
  after_all:
    - tool: delete_file
      args: { path: "res://test_output.tscn" }
    - tool: new_scene
      args: { root_type: Node }
```

## 测试文件组织

- 使用两位数字前缀命名文件: `00_meta.yaml`、`01_docs.yaml` 等
- 将相关的工具分组到同一个文件中
- 用 `headless: true` 标记 headless 兼容的测试
- 保持每个阶段专注于一个功能领域

## 运行单个测试

```bash
# 运行指定的测试文件
uv run python main.py test --file 00_meta.yaml

# 运行多个匹配的文件
uv run python main.py test --file "0*"

# 仅运行 headless 测试
uv run python main.py test --headless
```