# 测试框架总览

> C++ 进程内测试引擎（`TestEngine` + `/run-tests`）覆盖内置工具和 SDK 自定义工具，Python 编排器管理 Godot 生命周期。

## 架构

### C++ 测试引擎

```
                         ┌──────────────────────┐
                         │  .test.yaml 配置文件   │
                         └──┬───────────────────┘
                            │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
   ┌──────────────┐ ┌──────────┐ ┌────────────────┐
   │ C++ Test     │ │ 外部     │ │ 编辑器面板      │
   │ Engine       │ │ Python   │ │ TestRunnerDock  │
   │ (进程内)     │ │ 编排器   │ │ (C++ UI)        │
   └──────┬───────┘ └────┬─────┘ └───────┬────────┘
          └───────┬──────┘───────────────┘
                  ▼
         ┌──────────────────┐
         │  TestEngine::run │
         │  (YAML→执行→返回)│
         └──────────────────┘
```

- **进程内**：`TestEngine` 直接调用 `HandlerRegistry::execute()`，不经过 MCP 协议
- **双源入口**：C++ Dock 面板 + HTTP `POST /run-tests`
- **配置驱动**：YAML 文件定义测试，零脚本代码
- **统一框架**：内置工具和自定义工具走同一路径
- **磁盘校验**：支持 `.tscn`/`.tres`/`project.godot` 属性路径解析 + 类型转换 + 容差比较
- **自动清理**：EditorFileSystem 快照差分 + 工具返回值双源追踪，只删交集

## 设计原则

- **配置驱动**：YAML 测试文件完整描述测试，无需编写脚本
- **双源清理**：EditorFileSystem 内存遍历快照 + 工具返回值追踪，确保不误删用户文件
- **类型安全校验**：Godot Variant 类型转换 + 浮点容差，精确匹配引擎行为

## 入口

| 入口 | 方式 | 适用 |
|------|------|------|
| `POST /run-tests` (HTTP) | YAML 内容 → JSON 结果 | Python 编排器 / CI / curl |
| TestRunnerDock (C++ 面板) | 选择 YAML 文件 → 运行 | 编辑器内交互测试 |
| `uv run python tests/test_orchestrator.py` | Python 编排器 | CI / 开发调试 |

## 测试引擎详情

| 文档 | 说明 |
|------|------|
| [测试引擎](test-engine.md) | TestEngine 架构、执行流程、YAML 格式、磁盘校验、清理策略 |

## 运行

```bash
# C++ 测试引擎 (需要 Godot 运行中):
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/scene.test.yaml

# Python 编排器 (自动管理 Godot 生命周期):
uv run python tests/test_orchestrator.py

# 冒烟测试 (无需 Godot):
uv run python tests/smoke_test.py
```

## 依赖

- **C++ 引擎**：ryml (rapidyaml, 通过 CMake FetchContent)
- **Python 编排器**：mcp≥1.27, httpx≥0.27, pytest≥8.0, python-dotenv≥1.0, PyYAML≥6.0
- **Windows**：必须使用 `uv run python` 而非裸 `python`（Microsoft Store 路由桩会卡死）
