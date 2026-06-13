# 线程模型

> **一切都在 Godot 主线程上运行**——是项目最简单可靠的架构决策。

## 纯主线程

```mermaid
flowchart LR
    subgraph MAIN["Godot 主线程"]
        direction TB
        A["McpEditorPlugin::_process(delta)<br/>(EditorPlugin 虚函数，每帧调用)"]
        R["pending_restart_ 轮询"]
        C["HttpServer::poll()"]
        B["_try_bridge_connect()"]
        D["RuntimeBridge::poll()"]
    end

    A --> R
    A --> C
    A --> B
    A --> D
```

C++ 版本**没有任何工作线程**。所有操作（HTTP 解析、JSON 处理、命令执行、Godot API 调用）都在 `McpEditorPlugin::_process(delta)` 中同步完成（`editor_plugin.cpp:205-221`）。该函数是 `EditorPlugin` 的虚函数，由 Godot 引擎每帧自动调用。

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
// editor_plugin.cpp:205-221
void McpEditorPlugin::_process(double delta) {
    if (!started_) return;

    // 延迟重启轮询（等待 pending 请求完成或超时）
    if (pending_restart_) {
        bool timed_out = Time::get_singleton()->get_ticks_msec() / 1000.0 > restart_deadline_;
        if (force_restart_ || !mcp_handler_.has_pending_requests() || timed_out)
            restart_server(true);
    }

    http_server_.poll();
    _try_bridge_connect();
    runtime_bridge_.poll();
}
```

`HttpServer::poll()` 非阻塞地处理 TCP 连接（accept、read、parse、dispatch），`RuntimeBridge::poll()` 推进 TCP 客户端状态机。所有操作在单帧内同步完成。`http_server_` 内部有 `polling_` 标志防止 `EditorProgress` → `Main::iteration()` 导致的递归重入。

`HttpServer::start()` 签名（`http_server.hpp:40`）：

```cpp
Error start(uint16_t port, McpHandler *mcp_handler, const String &bind_address = "127.0.0.1");
```

返回 `Error`，调用方检查返回值（`editor_plugin.cpp:125`）。
