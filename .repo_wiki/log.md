# 变更日志

> 本文件为追加式记录，追踪项目 wiki 的每次更新。

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
