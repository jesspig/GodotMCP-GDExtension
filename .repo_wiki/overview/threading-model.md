# 线程模型

> **一切都在 Godot 主线程上运行**——是项目最简单可靠的架构决策。

## 纯主线程

```mermaid
flowchart LR
    subgraph MAIN["Godot 主线程"]
        direction TB
        A["EditorPlugin::_on_process_frame()<br/>(通过 process_frame 信号每帧调用)"]
        C["HttpServer::poll()"]
        D["接受连接 → 解析请求 → 查找 HandlerRegistry → 执行 ITool"]
    end
    
    A --> C
    C --> D
```

C++ 版本**没有任何工作线程**。所有操作（HTTP 解析、JSON 处理、命令执行、Godot API 调用）都在 `EditorPlugin::_on_process_frame()` 中同步完成，该函数通过 `SceneTree::process_frame` 信号每帧调用。

这意味着：
- **无需** `MainThreadDispatcher`
- **无需** 跨线程日志（直接调用 `UtilityFunctions::print`）
- **无需** tokio 运行时
- 无 `bind_mut` 死锁风险
- 所有 `cmd_*` 函数可以直接调用 Godot API

## 为何选择纯主线程

Godot GDExtension API 要求所有 API 调用发生在主线程。C++ godot-cpp 绑定没有额外的线程借用检查机制，因此只要保证所有代码跑在主线程即可。`extensions/src/` 通过 `_on_process_frame` 确保这一点。

## 实现细节（C++）

```cpp
// editor_plugin.cpp
// McpHandler 的 registry 指针通过构造函数传入：McpHandler mcp_handler_{&registry_}
void McpEditorPlugin::_enter_tree() {
    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", ""));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_itools(registry_);
    
    http_server_.start(http_port, &mcp_handler_);  // start(port, McpHandler*)
    
    SceneTree *tree = Object::cast_to<SceneTree>(get_tree());
    tree->connect("process_frame", callable_mp(this, &McpEditorPlugin::_on_process_frame));
}

void McpEditorPlugin::_on_process_frame() {
    if (!started_) return;
    http_server_.poll();  // HTTP: 解析请求 + MCP 会话 + SSE 刷新
}
```

`HttpServer` 通过 `McpHandler` 分发请求（标准 MCP JSON-RPC 2.0 会话管理），最终调用 `HandlerRegistry::find()` 查找并执行对应的 `ITool`。
