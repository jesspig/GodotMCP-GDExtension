# 变更日志

> 仅追加的项目变更记录（最新在前）。

## 2026-05-30 — Wiki 代码对齐修复

- **修复** 多处与实际代码不一致的地方
- **修复** `editor-plugin.md`：将不存在的 `mcp_handler_.set_registry()` 改为构造函数传递，修正 `http_server_.start()` 签名（只接受 port + McpHandler*）
- **修复** `ipc-bridge.md`：错误码改为 `mcp_handler.hpp` 中的实际常量（kParseError/kInvalidRequest/kMethodNotFound/kInvalidParams/kInternalError/kResourceNotFound/kServerTerminated）；移除未实现的分页声明
- **修复** `threading-model.md`：修正 `_enter_tree()` 代码片段
- **修复** `specification/project-structure.md` / `extensions/gdext.md` / `overview/architecture.md`：移除已不存在的 `protocol/` 目录引用
- **修复** `specification/ipc-protocol.md`：移除分页声明；错误处理表改用实际常量
- **补充** `reference/build-and-package.md`：添加 `deep-clean` CMake target 和 `build.py` stale cache 自动恢复说明

## 2026-05-30 — 移除 Python 服务器，统一为单进程架构

- **删除** `modules/server.md`（Python/Cython MCP 服务器文档）
- **删除** `reference/editor-control.md`（服务器端编辑器控制工具文档）
- **重写** `overview/architecture.md`：双进程 → 单进程（C++ GDExtension only，MCP Streamable HTTP :9600）
- **重写** `overview/threading-model.md`：移除 WsServer，仅保留 HttpServer poll
- **重写** `modules/ipc-bridge.md`：移除 WebSocket 路径 A，仅保留 MCP Streamable HTTP
- **重写** `specification/ipc-protocol.md`：移除 WebSocket IPC 协议，仅保留 HTTP JSON-RPC 2.0
- **重写** `reference/client-config.md`：stdio → streamable-http 配置
- **重写** `design/decisions.md`：更新 ADR 反映单进程架构
- **更新** `index.md`：移除 Python 服务器引用，更新描述
- **更新** `modules/editor-plugin.md`：移除 ws_server_ 引用
- **更新** `modules/command-routing.md`：移除 stdio/WebSocket 路径
- **更新** `modules/lsp-client.md`：移除 Python 服务器中转
- **更新** `modules/editor-control-gdext.md`：移除服务器端对比
- **更新** `modules/dock-ui.md`：WebSocket → HTTP
- **更新** `modules/csharp-solution.md`：移除端口 9500 引用
- **更新** `specification/project-structure.md`：移除 server/ 目录
- **更新** `reference/ci-cd.md`：移除 Python/Cython/server 引用
- **更新** `reference/build-and-package.md`：移除 Python 服务器构建流程
- **更新** `reference/tools-catalog.md`：移除服务器端工具（125 → 121）
- **更新** `reference/client-quirks.md`：更新为 Streamable HTTP
- **更新** `extensions/gdext.md`：移除 WsServer 和 Python 客户端引用

## 2026-05-29 — 全面更新：HTTP 服务器、工具计数修正、架构文档重构

- **新增记录**: MCP Streamable HTTP 传输（`HttpServer` :9600 + `McpHandler` JSON-RPC 2.0 会话管理）
- **修正工具计数**: 17 个处理器文件定义 121 个工具；16 组在 `register_all_tools()` 中活跃注册（115 个）；`register_script_cs`（6 个 C# 工具）已声明但未调用
- **修正 `IpcResponse.code` 类型**: `int` → `str`（Python protocol.py 中 `code` 是字符串）
- **修正路径引用**: `tools/patch_entry_c.py` → `server/tools/patch_entry_c.py`；`.venv` 位于项目根目录
- **修正 ProjectSettingsCommands**: 工具数从 3 更正为 7（含 autoload 和场景列表）
- **新增 ADR-010**: MCP Streamable HTTP 传输决策
- **新增 ADR-011**: `register_script_cs` 声明但未注册状态
- **更新架构总览**: 双传输路径图（stdio/WebSocket + HTTP 直连）
- **更新线程模型**: 双服务器 poll（WsServer + HttpServer）
- **更新 IPC 协议规范**: 两条路径的完整协议文档
- **更新 CI/CD**: `cd server && pytest` → `pytest`（从根目录运行）；`source .venv/bin/activate` → 无需激活
- **更新项目结构**: `pyproject.toml` 仅在根目录；移除 `server/pyproject.toml` 引用

## 2026-05-28 — 全面清理 Rust 遗留引用，对齐实际代码

- **删除** 4 个纯 Rust 遗留页面：`crates/core.md`、`crates/gdext.md`、`modules/dispatcher.md`、`specification/workspace.md`
- **新增** `modules/server.md`（从 `crates/server.md` 重写，移除 Rust 引用）
- **新增** `specification/project-structure.md`（替换 `workspace.md`，移除 Cargo 引用）
- **全量更新** 所有页面统一版本号为 `0.1.5-dev2`
- **修复** 工具计数：17 组 C++ 处理器（含 search），修正 tool counts 与实际 `registry.py` 一致
- **清理** 所有页面中的 Rust 语言/架构对比/代码示例引用
- **修复** 事实错误：`editor_ctl.py` 版本源（pyproject.toml 非硬编码）、`entry.py`（非 `.pyx`）、IPC 错误码字符串值、CI/CD `cargo test` → `pytest`
- **更新** `design/decisions.md` 中 ADR 以反映 Rust 代码已被完全移除

## 2026-05-27 — 知识库与 Rust 遗留代码对齐

- 完成了 `.repo_wiki/` 的完整协调，与当前 C++ 实现对齐
- 25 个 Wiki 页面更新/编写：架构总览、线程模型、各模块（日志、调度器、IPC、编辑器插件、场景命令、LSP 客户端、C# 解决方案、插件管理、输入映射、Dock UI 等）、引用文档、规范、设计决策
- 过时的 Rust 页面明确标记为"仅遗留/仅用于测试"
- 添加了 ADR-009 记录 C++ GDExtension 重写决策
- 构建系统页面更新（CMake + Cython 端，无 Corrosion）
- CI/CD 页面更新（无 cargo fmt/clippy）
- Workspace 页面更新（C++ 中心项目结构）

## 初始版本

- 使用 Rust `gdext` crate（v0.5）创建初始 Wiki 文档
- 文档涵盖 28 个文件、7 个目录
