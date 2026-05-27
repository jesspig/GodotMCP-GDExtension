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

## 初始版本

- 使用 Rust `gdext` crate（v0.5）创建初始 Wiki 文档
- 文档涵盖 28 个文件、7 个目录
