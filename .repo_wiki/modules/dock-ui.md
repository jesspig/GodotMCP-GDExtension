# Dock UI

> Godot 编辑器右侧面板。**当前未实现自定义 Dock UI**，计划见 `docs/plan/phase-3-dock-ui.md`。

当前 `McpEditorPlugin` 的 `_enter_tree()` 仅启动 HTTP 服务器并连接 `process_frame` 信号，不添加任何 Dock UI。
