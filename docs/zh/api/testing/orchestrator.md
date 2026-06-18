# 测试编排器

Python 测试编排器管理 Godot 编辑器进程并调度 YAML 测试文件。

## 命令

```bash
uv run python main.py test
```

## 选项

| 标志 | 描述 |
|------|-------------|
| `--file PATTERN` | 运行指定的测试文件（对 YAML 名称使用 glob 模式） |
| `--headless` | 仅运行标记为 `headless: true` 的测试 |
| `--keep-open` | 测试完成后保持 Godot 运行 |
| `--no-auto` | 不自动启动 Godot（连接到已在运行的实例） |

## 架构

```
test_orchestrator.py
    |
    +-- GodotManager (godot_manager.py)
    |     管理 Godot 进程生命周期
    |     - 以 --editor 启动（可选 --headless）
    |     - Ping MCP 端点直到就绪
    |     - 完成后结束进程
    |
    +-- YAML 发现
    |     - 扫描 tests/yaml_tests/
    |     - 按 headless 兼容性过滤
    |     - 按 --file 模式过滤
    |
    +-- 测试调度
    |     - POST 每个 YAML 到 /run-tests
    |     - 每个文件 120 秒超时
    |     - 失败时以非 headless 模式重试 headless 测试
    |
    +-- 报告 (report.py)
          - 生成 JSON + Markdown 报告
          - 各阶段的通过/失败/跳过统计
          - 按工具聚合
```

## 输出

结果写入 `tests/output/`:

```
tests/output/
  report.json           # 机器可读报告
  report.md             # 人类可读报告
  <timestamp>_*.json    # 每次运行的独立报告
```

## 报告结构

```json
{
  "phases": {
    "00_meta": {
      "pass": 7,
      "fail": 0,
      "skip": 0
    }
  },
  "summary": {
    "phases": 26,
    "pass": 150,
    "fail": 0,
    "skip": 3,
    "call_count": 300,
    "call_success": 298,
    "unique_tools": 100,
    "unique_success": 99,
    "duration_ms": 45000
  },
  "tool_stats": {
    "add_node": { "total": 10, "pass": 10, "fail": 0, "failing_phases": [] },
    "delete_node": { "total": 5, "pass": 4, "fail": 1, "failing_phases": ["04_scene_tree"] }
  }
}
```

## 添加测试文件

1. 在 `tests/yaml_tests/` 中创建新的 YAML 文件
2. 遵循 [管道格式](/api/testing/pipeline-format)
3. 运行: `uv run python main.py test --file my_new_test.yaml`