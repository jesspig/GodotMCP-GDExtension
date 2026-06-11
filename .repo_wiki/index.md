# Godot MCP — 项目知识库

> C++ **GDExtension** 单进程架构，通过 **MCP Streamable HTTP**（端口 9600）将 Godot 4.6+ 编辑器暴露给 AI 工具。使用 `godot-cpp 10.0.0-rc1`，X-macro 分文件注册，四层工具体系（语义专用 + 属性组 + 通用兜底 + 文档查询），Godot 内置文档驱动指令数据，rapidyaml（ryml）YAML 解析，内置 C++ 测试引擎。

## 项目快照

| 维度 | 当前状态 | 目标（ADR-015） |
|------|---------|----------------|
| C++ 源码根 | `extensions/src/` | 不变 |
| 注册方式 | `// @tool register` + codegen | **X-macro 分文件注册** |
| 工具总数 | ~11,791 | **~359**（↓97%） |
| 工具体系 | YAML 生成 get/set 工具 | **四层体系**：语义专用(~80) + 属性组(~126) + 通用兜底(4) + 文档(7) |
| 指令数据源 | YAML 数据库（需手动同步） | **Godot 内置 DocTools**（零维护） |
| 覆盖 | 100%（通过工具爆炸） | **100%**（通过 Layer 0 兜底） |
| 场景树工具 | `editor_tools/scene_tree/`（25+ 工具） | 保留 |
| 运行时桥接工具 | `runtime_tools/bridge/`（6 工具） | 保留 |
| SDK 层 | `extensions/src/sdk/`（`McpToolDefinition` + `McpToolRegistry`） | 保留，SDK 平权 |
| 测试框架 | C++ `TestEngine`（`/run-tests`）+ Python 编排器 | 保留 |
| HTTP 端口 | `:9600`（env `GODOT_MCP_HTTP_PORT` 覆盖） | 不变 |
| 桥接端口 | `:9601`（env `GODOT_MCP_BRIDGE_PORT` 覆盖） | 不变 |
| Pinned deps | `godot-cpp 10.0.0-rc1`、`ryml v0.7.0` | 不变 |
| 版本号 | 仅 `CMakeLists.txt:22` `PROJECT_VERSION` | 不变 |

## 快速导航

| 类别 | 文档 |
|------|------|
| 架构总览 | [overview/architecture.md](overview/architecture.md) · [overview/threading-model.md](overview/threading-model.md) |
| GDExtension 实现 | [extensions/gdext.md](extensions/gdext.md) |
| 代码生成 | [modules/codegen.md](modules/codegen.md) |
| 命令路由 | [modules/command-routing.md](modules/command-routing.md) |
| MCP 传输 | [modules/ipc-bridge.md](modules/ipc-bridge.md) · [specification/ipc-protocol.md](specification/ipc-protocol.md) |
| 运行时桥接 | [modules/runtime-bridge.md](modules/runtime-bridge.md) |
| 插件生命周期 | [modules/editor-plugin.md](modules/editor-plugin.md) · [modules/dock-ui.md](modules/dock-ui.md) |
| 元工具 | [modules/meta-tools.md](modules/meta-tools.md) |
| 工具实现模式 | [modules/scene-commands.md](modules/scene-commands.md) |
| 场景树工具 | [modules/scene-tree-tools.md](modules/scene-tree-tools.md) |
| 工作区工具 | [modules/workspace-tools.md](modules/workspace-tools.md) |
| 文件系统工具 | [modules/filesystem-tools.md](modules/filesystem-tools.md) |
| 项目设置工具 | [modules/settings-tools.md](modules/settings-tools.md) |
| 分组工具 | [modules/group-tools.md](modules/group-tools.md) |
| 信号工具 | [modules/signal-tools.md](modules/signal-tools.md) |
| 资源管理工具 | [modules/resource-tools.md](modules/resource-tools.md) |
| SDK 层 | `extensions/src/sdk/mcp_tool_definition.hpp` · `mcp_tool_registry.hpp`（详见源代码） |
| 节点属性系统 | `node_props/db/*.yaml`（283 文件）· `node_property_tool.hpp` |
| 资源属性系统 | [modules/resource-property-tools.md](modules/resource-property-tools.md) |
| 项目设置扩展（旧） | [modules/project-settings-ext.md](modules/project-settings-ext.md) |
| LSP 客户端 | [modules/lsp-client.md](modules/lsp-client.md) |
| 输入映射 | [modules/input-map.md](modules/input-map.md) |
| 插件管理 | [modules/plugin-management.md](modules/plugin-management.md) |
| 编辑器控制 | [modules/editor-control-gdext.md](modules/editor-control-gdext.md) |
| C# 解决方案 | [modules/csharp-solution.md](modules/csharp-solution.md) |
| 日志 | [modules/logging.md](modules/logging.md) |
| 构建与打包 | [reference/build-and-package.md](reference/build-and-package.md) |
| CI/CD | [reference/ci-cd.md](reference/ci-cd.md) |
| 客户端配置 | [reference/client-config.md](reference/client-config.md) · [client-quirks.md](reference/client-quirks.md) |
| 工具目录 | [reference/tools-catalog.md](reference/tools-catalog.md) |
| 项目结构 | [specification/project-structure.md](specification/project-structure.md) |
| 设计决策（ADR） | [design/decisions.md](design/decisions.md) |
| 优化路线图（含追踪清单） | [design/roadmap.md](design/roadmap.md) |
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
3. **阅读 `design/decisions.md#ADR-015`** — 理解四层工具体系、X-macro 注册、Godot 内置文档驱动
4. **阅读 `modules/runtime-bridge.md`** — 理解运行时桥接设计（GameBridgeNode + RuntimeBridge）
5. **添加新工具时**：
   - 创建 `.hpp` 文件实现 `ITool` 接口
   - 在 `extensions/src/built_in/tools/register/` 下对应分类的 X-macro 文件加一行
   - 不需要 `// @tool register` 注释，不需要运行 codegen
6. **运行测试前**：见 `AGENTS.md`「测试」章节 + `tests/.env` 配置
7. **遇到具体模块问题**：上表点击对应模块文档

## 给 Agent 的提醒

- **入口符号** `gdext_mcp_init`（`register_types.cpp:45`）
- **不要修改** `extensions/CMakeLists.txt:15` 的 `GODOTCPP_API_VERSION "4.6"` 与根 `CMakeLists.txt:72` 的 `compatibility_minimum = "4.6"` 之间的绑定
- **升级 godot-cpp / ryml 前必测** —— 二者均为 FetchContent 拉取，缓存键与版本绑定
- **MSVC UTF-8** —— 含非 ASCII 的字符串字面量必须用 `String::utf8("中文")`，根 `CMakeLists.txt:43` 已加 `/utf-8 /bigobj`
- **DLL 文件锁** —— Godot 编辑器持有 `example/addons/godot_mcp/bin/godot_mcp_gdext.dll`，重建失败时先关闭编辑器或禁用插件
- **构建优化** —— sccache/ccache、Unity jumbo build、lld-link 均已在 CMakeLists.txt 中配置，增量构建默认加速
