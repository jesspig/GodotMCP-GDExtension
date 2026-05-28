# 变更日志

> 仅追加的项目变更记录。

## 2026-05-27 — 知识库与 Rust 遗留代码对齐

- 完成了 `.repo_wiki/` 的完整协调，与当前 C++ 实现对齐
- 25 个 Wiki 页面更新/编写：架构总览、线程模型、各模块（日志、调度器、IPC、编辑器插件、场景命令、LSP 客户端、C# 解决方案、插件管理、输入映射、Dock UI 等）、引用文档、规范、设计决策
- 过时的 Rust 页面明确标记为"仅遗留/仅用于测试"
- 添加了 ADR-009 记录 C++ GDExtension 重写决策
- 构建系统页面更新（CMake + Cython 端，无 Corrosion）
- CI/CD 页面更新（无 cargo fmt/clippy）
- Workspace 页面更新（C++ 中心项目结构）

## 2026-05-28 — 全面清理 Rust 遗留引用，对齐实际代码

- **删除** 4 个纯 Rust 遗留页面：`crates/core.md`、`crates/gdext.md`、`modules/dispatcher.md`、`specification/workspace.md`
- **新增** `modules/server.md`（从 `crates/server.md` 重写，移除 Rust 引用）
- **新增** `specification/project-structure.md`（替换 `workspace.md`，移除 Cargo 引用）
- **全量更新** 所有页面统一版本号为 `0.1.5-dev2`
- **修复** 工具计数：17 组 C++ 处理器（含 search），修正 tool counts 与实际 `registry.py` 一致
- **清理** 所有页面中的 Rust 语言/架构对比/代码示例引用
- **修复** 事实错误：`editor_ctl.py` 版本源（pyproject.toml 非硬编码）、`entry.py`（非 `.pyx`）、IPC 错误码字符串值、CI/CD `cargo test` → `pytest`
- **更新** `design/decisions.md` 中 ADR 以反映 Rust 代码已被完全移除

## 初始版本

- 使用 Rust `gdext` crate（v0.5）创建初始 Wiki 文档
- 文档涵盖 28 个文件、7 个目录
