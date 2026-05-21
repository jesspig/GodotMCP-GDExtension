# 变更日志

> 本文件为追加式记录，追踪项目 wiki 的每次更新。

## [2026-05-22] wiki | 全量重构 — 按模块拆分，对齐 35 工具 + Pump 架构

- 删除全部历史目录（`overview/`, `implementation/`, `design/`, `planning/`, `guide/`, `specification/`）并重建为新结构：`overview/`, `crates/`, `modules/`, `reference/`, `design/`, `specification/`。
- 新增/重写 14 页面，全部依据 2026-05-22 代码事实：
  - `index.md`、`overview/architecture.md`、`overview/threading-model.md`（pump 模式 + tokio↔主线程双管道，最关键页面）
  - `crates/{core,server,gdext}.md`（每 crate 的文件级地图）
  - `modules/{dispatcher,logging,editor-plugin,ipc-bridge,command-routing,scene-commands,dock-ui}.md`
  - `reference/{tools-catalog,client-config,client-quirks,build-and-package}.md`
  - `specification/{ipc-protocol,workspace}.md`
  - `design/decisions.md`（10 条 ADR）
- 已知不准确事实清算（保留为新页面里显式标注）：旧 `current-status.md` 还在说"4 个工具"、`tool_manager.rs` 写"Tools (4/4)"、`integration.rs` 按钮无回调、`settings.rs` 端口只读 —— 全部在 `modules/dock-ui.md` 中明确说明为 skeleton。
- 工具数量校准为 **35**（4 meta + 31 scene），匹配 `tool_registry.rs::register_defaults` 与 `commands/scene.rs::TOOL_NAMES`。
- 删除所有 Phase 1/2a/2b 路线图说法 —— 路线图属于项目管理，不属于知识库。

## [2026-05-21] wiki | Wiki 刷新 — 事实校准 + 实现文档

- 校准 `specification/workspace.md`：修正依赖列表（godot `default` 非 `codegen`、移除 `parking_lot`/`tracing`、uuid 含 `serde` feature）、修正 entry_symbol 为 `gdext_rust_init`、新增实际 vs 计划目录树对比
- 重写 `design/ipc-bridge.md`：将模型代码替换为实际实现（`IpcWebSocketServer` 构造函数、`PluginState` 替代 `parking_lot::RwLock`、无 `CommandHandler`）、标注计划功能
- 校准 `specification/protocol.md`：移除对不存在文件（`error.rs`）的引用、修正 IPC 消息格式为平坦 params、移除未实现的 resources/tool_router 代码、标注 Streamable HTTP 为计划
- 重写 `overview/architecture.md`：在 mermaid 图中标注 ✅/📋/❌ 状态、新增实现状态总览表
- 新建 `implementation/` 目录：`current-status.md`（Phase 1 完成度 + 已知缺失）、`gdext-skeleton.md`（EditorPlugin + WS Server 详细文档）、`server-skeleton.md`（CLI + ServerHandler + GodotBridge 详细文档）
- 更新 `index.md`：新增实现文档分区
- 更新 `AGENTS.md`：替换"无代码"为准确状态表、修正依赖 feature、添加 `package_addons.py` 命令

## [2026-05-20] 规划 | 12 客户端适配 + 个体工具粒度 + Godot 4.6

- 扩展客户端列表至 12 个：Claude Code, Codex, Gemini CLI, OpenCode, Cursor, GitHub Copilot, Qwen Code, Trae, Trae CN, Qoder, Antigravity, CodeBuddy
- 每客户端支持独立协议选择（stdio / HTTP），UI 使用 OptionButton 下拉
- 记录各客户端 JSON key 差异：GitHub Copilot 用 `servers`，Antigravity 用 `serverUrl`，Gemini CLI/Qwen Code 用 `httpUrl`
- 工具热切换从分类粒度升级为**个体工具粒度**：每个工具可独立启用/禁用
- ToolManifest/ToolInfo 增加 `enabled` 字段，ToolRegistry 使用 DashMap 管理运行时状态
- Dock UI Tools 区域改为两级展开（分类 → 个体工具 CheckBox）
- Server 端 register_tools 改为按工具名精确注册/移除
- 更新 `compatibility_minimum` 从 4.3 至 4.6（匹配 gdext v0.5 默认 API level）
- 更新文件：`dock-ui.md`, `client-config.md`, `tools.md`, `workspace.md`, `AGENTS.md`

## [2026-05-20] 规划 | 初始 Wiki 建立

- 完成三项调研：
  - Unity MCP 官方方案（relay + IPC 桥接）
  - CoplayDev/unity-mcp（C# + Python uvx、HTTP+stdio、90+工具）
  - hi-godot/godot-ai（GDScript + Python FastMCP、120+操作、WebSocket）
  - elfensky/godot-mcp（GDScript + Node.js、61工具、双模式）
- 确定纯 Rust 技术栈：`godot-rust/gdext` + `rmcp`
- 确定三协议策略：stdio + Streamable HTTP（不含旧版 SSE）
- 确定产出物：`godot-mcp-server` 二进制 + `addons.zip` GDExtension 插件包
- 设计独立 Dock UI 面板，支持工具热切换
- 编写此初始 Wiki（10 页面，6 子目录）