# 线程模型

> **一切都在 Godot 主线程上运行**——是项目最简单可靠的架构决策。

## 纯主线程

```mermaid
flowchart LR
    subgraph MAIN["Godot 主线程"]
        direction TB
        A["EditorPlugin::_on_process_frame()<br/>(通过 process_frame 信号每帧调用)"]
        B["WsServer::poll()"]
        C["接受新连接 → 处理 JSON 请求 → 查找 HandlerRegistry → 执行 CommandFn"]
    end
    
    A --> B
    B --> C
```

C++ 版本**没有任何工作线程**。所有操作（WebSocket 接受、JSON 解析、命令执行、Godot API 调用）都在 `EditorPlugin::_on_process_frame()` 中同步完成，该函数通过 `SceneTree::process_frame` 信号每帧调用。

这意味着：
- **无需** `MainThreadDispatcher`
- **无需** 跨线程日志（直接调用 `UtilityFunctions::print`）
- **无需** tokio 运行时
- 无 `bind_mut` 死锁风险
- 所有 `cmd_*` 函数可以直接调用 Godot API

## 为何选择纯主线程

Godot GDExtension API 要求所有 API 调用发生在主线程。C++ godot-cpp 绑定没有额外的线程借用检查机制，因此只要保证所有代码跑在主线程即可。`extensions/gdext/` 通过 `_on_process_frame` 确保这一点。

相比之下，Rust 的 `gdext` crate 的 `Gd<T>::bind_mut()` 借用机制增加了线程复杂性，是项目从 Rust 迁移到 C++ 的关键动因之一。

## 实现细节（C++）

```cpp
// editor_plugin.cpp
void McpEditorPlugin::_enter_tree() {
    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", ""));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_all_tools(registry_);
    
    ws_server_.start(port, &registry_);
    
    // connect to SceneTree::process_frame
    SceneTree *tree = Object::cast_to<SceneTree>(get_tree());
    tree->connect("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame));
}

void McpEditorPlugin::_on_process_frame() {
    if (!started_) return;
    ws_server_.poll();  // 接受连接 + 处理消息 + 执行命令, 全部同步
}
```


