# 变更日志

> 仅追加的项目变更记录（最新在前）。

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
