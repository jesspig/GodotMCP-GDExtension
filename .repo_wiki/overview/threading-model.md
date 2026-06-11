# 线程模型

> **一切都在 Godot 主线程上运行**——是项目最简单可靠的架构决策。

## 纯主线程

```mermaid
flowchart LR
    subgraph MAIN["Godot 主线程"]
        direction TB
        A["McpEditorPlugin::_process(delta)<br/>(EditorPlugin 虚函数，每帧调用)"]
        C["HttpServer::poll()"]
        B["_try_bridge_connect()"]
        D["RuntimeBridge::poll()"]
    end
    
    A --> C
    A --> B
    A --> D
```

C++ 版本**没有任何工作线程**。所有操作（HTTP 解析、JSON 处理、命令执行、Godot API 调用）都在 `McpEditorPlugin::_process(delta)` 中同步完成。该函数是 `EditorPlugin` 的虚函数，由 Godot 引擎每帧自动调用。

这意味着：
- **无需** `MainThreadDispatcher`
- **无需** 跨线程日志（直接调用 `UtilityFunctions::print`）
- **无需** tokio 运行时
- 无 `bind_mut` 死锁风险
- 所有 `cmd_*` 函数可以直接调用 Godot API

## 为何选择纯主线程

Godot GDExtension API 要求所有 API 调用发生在主线程。C++ godot-cpp 绑定没有额外的线程借用检查机制，因此只要保证所有代码跑在主线程即可。`extensions/src/` 通过重写 `_process()` 确保这一点。

## 实现细节（C++）

```cpp
// editor_plugin.cpp
void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;

    registry_.set_engine_version(Engine::get_singleton()->get_version_info().get("string", ""));
    registry_.set_plugin_version(String(GODOT_MCP_PLUGIN_VERSION));
    register_itools(registry_);

    // 初始化 SDK
    McpToolRegistry *sdk_reg = McpToolRegistry::get_singleton();
    sdk_reg->set_handler_registry(&registry_);
    sdk_reg->set_mcp_handler(&mcp_handler_);

    int http_port = read_port_from_env("GODOT_MCP_HTTP_PORT", 9600);
    http_server_.start(http_port, &mcp_handler_);

    started_ = true;
}

void McpEditorPlugin::_process(double delta) {
    if (!started_) return;
    http_server_.poll();
    _try_bridge_connect();
    runtime_bridge_.poll();
}
```

`HttpServer::poll()` 非阻塞地处理 TCP 连接（accept、read、parse、dispatch），`RuntimeBridge::poll()` 推进 TCP 客户端状态机。所有操作在单帧内同步完成。`http_server_` 内部有 `polling_` 标志防止 `EditorProgress` → `Main::iteration()` 导致的递归重入。
