# Dock UI 实现文档

> Phase 2a 交付。反映当前代码真实状态。

## 模块结构

```
crates/gdext/src/dock/
├── mod.rs              # 模块导出
├── main_dock.rs        # 主容器创建函数
├── status_bar.rs       # 状态栏组件
├── tool_manager.rs     # 工具管理面板（交互完整）
├── integration.rs      # 客户端集成面板（骨架）
└── settings.rs         # 高级设置面板（骨架）
```

## 主容器 (`main_dock.rs`)

`create_dock(broadcast_tx)` 返回 `Gd<VBoxContainer>`，包含 4 个子面板：
1. 状态栏
2. 客户端集成面板
3. 工具管理面板
4. 高级设置面板

通过 `EditorPlugin::add_control_to_dock(DockSlot::RIGHT_UL, &dock)` 添加到编辑器右侧。

## 状态栏 (`status_bar.rs`)

- 绿色 `ColorRect` 指示灯（12x12）
- `Label` 显示 "Status: Running"
- `Button` 显示 "Stop"（禁用，Phase 3 实现）

## 工具管理面板 (`tool_manager.rs`)

**交互完整**：用户可勾选/取消工具，变更通过 IPC 广播到 Server。

- 4 个 `CheckBox`，每个对应一个工具
- 勾选时触发 `toggled` 信号 → `on_tool_toggled` → 广播 `tool_list_updated` 通知
- Server 收到通知后更新 `ToolRegistry`，下次 `list_tools` 返回更新后的列表

## 客户端集成面板 (`integration.rs`)

**骨架**：12 个 `Button`（禁用），tooltip 显示 "Phase 3 实现"。

## 高级设置面板 (`settings.rs`)

**骨架**：WS 端口/HTTP 端口 `LineEdit`（禁用）+ 操作按钮（禁用）。
