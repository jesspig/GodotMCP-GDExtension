# 编辑器插件（`McpEditorPlugin`）

> `godot_mcp_gdext.dll` 的生命周期管理。

### 生命周期

```mermaid
stateDiagram-v2
    [*] --> EnterTree: Godot 加载插件
    
    EnterTree --> Initializing: _enter_tree()
    
    state Initializing {
        [*] --> ReadVersion: 读取引擎版本 + 插件版本
        ReadVersion --> RegisterTools: register_itools(registry_)  via codegen
        RegisterTools --> InitSDK: 初始化 McpToolRegistry 单例
        InitSDK --> ReadPort: 读取端口
        ReadPort --> StartServer: http_server_.start()
        StartServer --> ConnectProcessFrame: connect("process_frame", callable_mp)
        ConnectProcessFrame --> [*]
    }
    
    Initializing --> Running: 插件就绪
    
    Running --> Processing: _process(delta) 每帧调用
    
    state Processing {
        [*] --> PollHTTP: http_server_.poll()
        PollHTTP --> TryBridge: _try_bridge_connect()
        TryBridge --> PollBridge: runtime_bridge_.poll()
        PollBridge --> [*]
    }
    
    Processing --> Exiting: 编辑器卸载 / _exit_tree()
    
    state Exiting {
        [*] --> Disconnect: disconnect process_frame
        Disconnect --> StopServers: http_server_.stop()
        StopServers --> [*]
    }
    
    Exiting --> [*]
```

### `_enter_tree()` 初始化

```cpp
// McpEditorPlugin 的 McpHandler 通过构造函数传入 registry_ 指针：
// McpHandler mcp_handler_{&registry_};

void McpEditorPlugin::_enter_tree() {
    if (!Engine::get_singleton()->is_editor_hint()) return;
    
    registry_.set_engine_version(...);     // 引擎版本
    registry_.set_plugin_version(GODOT_MCP_PLUGIN_VERSION);  // 编译时版本

    register_itools(registry_);            // codegen 生成，注册所有 ITool + YAML 工具
    
    // 初始化 SDK 单例
    McpToolRegistry *sdk_reg = McpToolRegistry::get_singleton();
    sdk_reg->set_handler_registry(&registry_);
    sdk_reg->set_mcp_handler(&mcp_handler_);
    
    int http_port = read_port_from_env("GODOT_MCP_HTTP_PORT", 9600);
    
    if (!http_server_.start(http_port, &mcp_handler_)) return;
    
    // 创建 TestRunnerDock
    test_dock_ = memnew(TestRunnerDock);
    test_dock_->set_test_engine(&test_engine_);
    // add_control_to_bottom_panel 在 godot-cpp 10.0.0-rc1 未绑定，用 call() 兜底
    if (has_method("add_control_to_bottom_panel")) {
        call("add_control_to_bottom_panel", test_dock_, "Tests");
    } else {
        add_child(test_dock_);
    }
    
    started_ = true;
}
```

### `_process()` 每帧执行

```cpp
void McpEditorPlugin::_process(double delta) {
    if (!started_) return;
    http_server_.poll();           // MCP HTTP: 解析 HTTP 请求 + 会话管理 + SSE 刷新
    _try_bridge_connect();         // 检测游戏启停，自动连接/断开 RuntimeBridge
    runtime_bridge_.poll();        // 驱动桥接连接状态
}
```

### `_exit_tree()` 清理

```cpp
void McpEditorPlugin::_exit_tree() {
    if (!started_) return;
    runtime_bridge_.disconnect();  // 断开运行时桥接
    http_server_.stop();           // 停止 HTTP 服务器
    // 移除底部面板
    if (test_dock_) {
        if (has_method("remove_control_from_bottom_panel")) {
            call("remove_control_from_bottom_panel", test_dock_);
        } else if (test_dock_->get_parent()) {
            test_dock_->get_parent()->remove_child(test_dock_);
        }
        test_dock_ = nullptr;
    }
    started_ = false;
}
```

### 关键设计

- **HTTP 服务器**: HttpServer (`:9600`, MCP Streamable HTTP)
- **端口**：通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖
- **`_process()` 驱动轮询**：`EditorPlugin::_process()` 在场景播放时停止触发，但 McpEditorPlugin 通过 `ei->is_playing_scene()` 检测游戏运行状态，在游戏运行时仍能正确维护桥接连接。`http_server_.poll()` + `runtime_bridge_.poll()` + `_try_bridge_connect()` 三合一。
- **运行时桥接**：`_try_bridge_connect()` 每帧检测 `ei->is_playing_scene()`，自动管理 `RuntimeBridge` 连接生命周期。`RuntimeBridge` 通过 TCP :9601 与游戏进程内的 `GameBridgeNode` 通信。
- **启动条件**：`EditorPlugin::_enter_tree()` 首先检查 `Engine::get_singleton()->is_editor_hint()`——非编辑器模式直接返回
- **Schema 自描述**: 每个 ITool 通过 `input_schema()` 提供自身 JSON Schema，无需外部配置文件
- **TestRunnerDock**: 编辑器底部面板，提供 GUI 方式运行 YAML 测试，与 C++ `TestEngine` 集成
- **SDK 初始化**: `McpToolRegistry` 单例在初始化时注入 `HandlerRegistry` 和 `McpHandler` 指针，供 GDScript/C# 自定义工具使用
