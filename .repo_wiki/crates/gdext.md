# `crates/gdext` — GDExtension Rust 实现（遗留）

> **⚠️ 仅用于测试。** 当前活跃的 GDExtension 是 C++ 实现（`extensions/gdext/`）。此 Rust cdylib 仅通过 `cargo test --workspace` 测试时编译。部署时使用 C++ 版本。

## 目录映射

| 目录/文件 | 内容 | 工具数 |
|-----------|------|--------|
| `commands/meta.rs` | ping, get_engine_version, get_plugin_version | 3 |
| `commands/node.rs` | 节点 CRUD、场景树、组管理、transform | 17 |
| `commands/property.rs` | 2D 属性 get/set | 19 |
| `commands/property_3d.rs` | 3D 属性 get/set | 6 |
| `commands/scene.rs` | 场景文件、编辑器标签操作、autoload | 15 |
| `commands/collision.rs` | 碰撞体添加 | 2 |
| `commands/find.rs` | 节点搜索 | 4 |
| `commands/script_helpers.rs` | call_method, get/set_variable | 3 |
| `commands/project_settings.rs` | 项目设置读写 | 7 |
| `commands/script_gd.rs` | GDScript 文件操作 | 5 |
| `commands/script_cs.rs` | C# 文件操作、Solution 生成、build | 6 |
| `commands/search.rs` | 文件搜索与替换 | 3 |
| `commands/undo.rs` | 撤销/重做 | 2 |
| `commands/editor_control.rs` | play_scene, stop, refresh, get_editor_info | 6 |
| `commands/project_settings_ext.rs` | 显示/物理/渲染/层名设置 | 10 |
| `commands/plugin_management.rs` | 列出/启用/禁用插件 | 2 |
| `commands/input_map.rs` | 输入动作管理 | 4 |

**总计: 121 个 gdext 工具 + 4 个服务器端工具 = 125**

## 工具注册

`commands/mod.rs` 中的 `create_registry()` 构建 17 个 CommandHandler 组（C++ 版本合并为 16 组）：

```
create_registry():
  MetaCommands(3) + 17 组 = 17 handler
```

`ws_server.rs::dispatch()` 先独立处理 MetaCommands（同步，无需 MainThreadDispatcher），再迭代 registry。

## 线程模型（区别于 C++ 的关键点）

Rust 版本使用双线程架构：

1. **tokio 工作线程**（2 核）：处理 WebSocket、JSON 解析、路由逻辑
2. **Godot 主线程**：执行所有 `cmd_*` 函数（调度到主线程）

详见[线程模型页](../overview/threading-model.md)。

## 测试

Rust 测试覆盖了协议类型和 Rust gdext 处理器：

```bash
cargo test --workspace
```

这些测试在 CI 中运行，不需要 Godot 编辑器。C++ 实现暂没有自动化测试。
