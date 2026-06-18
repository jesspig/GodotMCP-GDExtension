# 测试概述

GodotMCP 包含一个内置的基于 YAML 的测试引擎，在 Godot 编辑器进程内运行。测试定义为 YAML 管道文件，通过 HTTP POST 发送到 `/run-tests` 端点。

## 架构

```
YAML 管道文件
    |
    v
POST /run-tests (HTTP)
    |
    v
TestEngine::run(yaml)
    |
    v
PipelineParser -> PipelineDef  (解析 + 验证)
    |
    v
PipelineRunner::run(def)       (执行阶段/步骤)
    |
    v
断言 + 磁盘验证
    |
    v
JSON 结果报告
```

## 组件

| 组件 | 语言 | 位置 | 描述 |
|-----------|----------|----------|-------------|
| TestEngine | C++ | `extensions/src/testing/test_engine.hpp` | 顶层引擎 |
| PipelineParser | C++ | `extensions/src/testing/pipeline_parser.hpp` | YAML -> PipelineDef |
| PipelineRunner | C++ | `extensions/src/testing/pipeline_runner.hpp` | 管道执行器 |
| PipelineContext | C++ | `extensions/src/testing/pipeline_context.hpp` | 跨步骤数据存储 |
| TestOrchestrator | Python | `tests/test_orchestrator.py` | Godot 生命周期 + 测试调度 |
| YAML 测试 | YAML | `tests/yaml_tests/`（26 个文件） | 测试定义 |

## 测试工作方式

1. YAML 文件定义一个包含阶段和步骤的 **管道**
2. 每个步骤调用一个工具，带有特定参数
3. 使用 **断言系统** 定义预期结果
4. **磁盘验证** 可以检查场景文件、项目设置和文本文件
5. 运行器按顺序执行步骤，支持依赖和重试

## 运行测试

### 通过命令行

```bash
uv run python main.py test
```

### 通过 HTTP（需 Godot 在运行）

```bash
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/00_meta.yaml
```

### 选项

| 标志 | 描述 |
|------|-------------|
| `--file PATTERN` | 运行指定的测试文件（glob） |
| `--headless` | 仅运行 headless 兼容的测试 |
| `--keep-open` | 测试后保持 Godot 打开 |
| `--no-auto` | 不自动启动 Godot |