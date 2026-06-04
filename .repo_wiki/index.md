# Godot MCP — 项目知识库

> C++ **GDExtension** 单进程架构，通过 **MCP Streamable HTTP**（端口 9600）将 Godot 4.6+ 编辑器暴露给 AI 工具。使用 `godot-cpp 10.0.0-rc1`，`// @tool register` + codegen 自动注册工具，rapidyaml（ryml）YAML 解析，内置 C++ 测试引擎。
>
> **架构演化（重要）**：项目已完成从「自由函数 + 17 个 `register_<group>()` 显式注册」到「`ITool` 接口 + codegen 注释驱动自动注册」的统一重构。当前源码根为 `extensions/src/`（不再有 `extensions/gdext/`、`commands/cmd_*.cpp`、旧 17 组工具目录）。

## 项目快照

| 维度 | 状态 |
|------|------|
| C++ 源码根 | `extensions/src/` |
| ITool 头文件 | `extensions/src/built_in/tools/**/*.hpp`（19 个 `.hpp` 标 `// @tool register`） |
| 节点属性工具 | `extensions/src/built_in/tools/node_props/db/*.yaml`（283 节点类型） |
| 资源属性工具 | `extensions/src/built_in/tools/node_resource/db/*.yaml`（419 资源类型） |
| SDK 层 | `extensions/src/sdk/`（`McpToolDefinition` + `McpToolRegistry`） |
| 测试框架 | C++ `TestEngine`（`/run-tests`）+ Python 编排器（`test_orchestrator.py`） |
| 端口 | `:9600`（env `GODOT_MCP_HTTP_PORT` 覆盖） |
| Pinned deps | `godot-cpp 10.0.0-rc1`、`ryml v0.7.0` |
| 版本号 | 仅 `CMakeLists.txt:22` `PROJECT_VERSION`（CMake 自动生成 `plugin.cfg` + `.gdextension`） |

## 快速导航

| 类别 | 文档 |
|------|------|
| 架构总览 | [overview/architecture.md](overview/architecture.md) · [overview/threading-model.md](overview/threading-model.md) |
| GDExtension 实现 | [extensions/gdext.md](extensions/gdext.md) |
| 命令路由 | [modules/command-routing.md](modules/command-routing.md) |
| MCP 传输 | [modules/ipc-bridge.md](modules/ipc-bridge.md) · [specification/ipc-protocol.md](specification/ipc-protocol.md) |
| 插件生命周期 | [modules/editor-plugin.md](modules/editor-plugin.md) · [modules/dock-ui.md](modules/dock-ui.md) |
| 工具实现模式 | [modules/scene-commands.md](modules/scene-commands.md) |
| 信号工具 | [modules/signal-tools.md](modules/signal-tools.md) |
| 资源管理工具 | [modules/resource-tools.md](modules/resource-tools.md) |
| SDK 层 | `extensions/src/sdk/mcp_tool_definition.hpp` · `mcp_tool_registry.hpp`（详见源代码） |
| 节点属性系统 | `node_props/db/*.yaml`（283 文件）· `node_property_tool.hpp` |
| 资源属性系统 | [modules/resource-property-tools.md](modules/resource-property-tools.md) |
| LSP 客户端 | [modules/lsp-client.md](modules/lsp-client.md) |
| 输入映射 | [modules/input-map.md](modules/input-map.md) |
| 插件管理 | [modules/plugin-management.md](modules/plugin-management.md) |
| 项目设置扩展 | [modules/project-settings-ext.md](modules/project-settings-ext.md) |
| 编辑器控制 | [modules/editor-control-gdext.md](modules/editor-control-gdext.md) |
| C# 解决方案 | [modules/csharp-solution.md](modules/csharp-solution.md) |
| 日志 | [modules/logging.md](modules/logging.md) |
| 构建与打包 | [reference/build-and-package.md](reference/build-and-package.md) |
| CI/CD | [reference/ci-cd.md](reference/ci-cd.md) |
| 客户端配置 | [reference/client-config.md](reference/client-config.md) · [client-quirks.md](reference/client-quirks.md) |
| 工具目录 | [reference/tools-catalog.md](reference/tools-catalog.md) |
| 项目结构 | [specification/project-structure.md](specification/project-structure.md) |
| 设计决策（ADR） | [design/decisions.md](design/decisions.md) |
| 测试框架总览 | [testing/overview.md](testing/overview.md) |
| C++ 测试引擎 | [testing/test-engine.md](testing/test-engine.md) |
| Python 编排器 | [testing/orchestrator.md](testing/orchestrator.md) |
| 变更日志 | [log.md](log.md) |

## 已废弃文档（仅供历史参考）

| 文档 | 原因 |
|------|------|
| `testing/phase-system.md` | 18 个 `phase_XX_*.py` 阶段文件已于 2026-06 阶段 1 清理中删除。YAML `TestEngine` 为唯一测试路径。 |
| `testing/file-verifier.md` | `file_verifier.py` 已被 C++ `godot_file_verifier.hpp` 替代。 |
| `testing/mcp-client.md` | `mcp_client.py` 已被删除（仅 Python MCP fallback 路径使用）。`test_orchestrator.py` 通过 `/run-tests` HTTP 端点而非 MCP 客户端运行测试。 |

## Agent 上手指南

1. **从 `overview/architecture.md` 开始** — 理解单进程架构、数据流、目录布局
2. **阅读 `modules/command-routing.md`** — 理解 ITool 接口 + HandlerRegistry 调度
3. **添加新工具时**：见 AGENTS.md「添加内置工具」章节 + `extensions/src/built_in/tools/<dir>/<tool>.hpp` 现有样例
4. **运行测试前**：见 AGENTS.md「测试」章节 + `tests/.env` 配置
5. **遇到具体模块问题**：上表点击对应模块文档

## 给 Agent 的提醒

- **不要修改** `extensions/src/register_types.cpp:45` 的入口符号 `gdext_rust_init`（遗留名，`.gdextension` 引用）
- **不要修改** `extensions/CMakeLists.txt:15` 的 `GODOTCPP_API_VERSION "4.6"` 与根 `CMakeLists.txt:60` 的 `compatibility_minimum = "4.6"` 之间的绑定
- **升级 godot-cpp / ryml 前必测** —— 二者均为 FetchContent 拉取，缓存键与版本绑定
- **MSVC UTF-8** —— 含非 ASCII 的字符串字面量必须用 `String::utf8("中文")`
- **DLL 文件锁** —— Godot 编辑器持有 `addons/bin/godot_mcp_gdext.dll`，重建失败时先关闭编辑器
