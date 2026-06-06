# 架构概览

## 整体架构

```mermaid
graph TB
    subgraph AI_CLIENT["AI 客户端"]
        CC[Claude Code]
        CO[Continue]
        OC[opencode]
    end

    subgraph GODOT["Godot 编辑器进程"]
        EP[McpEditorPlugin<br/>EditorPlugin]
        HS[HttpServer<br/>TCPServer + HTTP/1.1]
        MH[McpHandler<br/>JSON-RPC 2.0 + SSE]
        HR[HandlerRegistry<br/>CommandFn 函数指针]
        LC[LspClient<br/>StreamPeerTCP]
        EU[EditorUndoRedoManager]

        EP --> HS
        HS --> MH
        MH --> HR
        LC -.->|LSP :6005| GodotLSP[Godot 内置 LSP]

        HR --> cmd_node[cmd_node: 21]
        HR --> cmd_scene[cmd_scene: 16]
        HR --> cmd_prop[cmd_property: 21]
        HR --> cmd_prop3d[cmd_property_3d: 6]
        HR --> cmd_script[cmd_script: 13]
        HR --> cmd_search[cmd_search: 3]
        HR --> cmd_other[... 10+ 组]
    end

    AI_CLIENT -- "Streamable HTTP :9600" --> HS
    cmd_node --> EU
    cmd_scene --> EU
    cmd_prop --> EU
```

## 核心设计原则

### 纯主线程

整个 GDExtension 运行在 Godot 编辑器的主线程上，**无工作线程、无锁**。编辑器帧回调 `_on_process_frame()` 驱动 `HttpServer::poll()` 处理请求。

```mermaid
flowchart LR
    subgraph MAIN["Godot 主线程"]
        A["_on_process_frame()"]
        C["HttpServer::poll()"]
        D["接收连接 → 解析请求 → 执行 CommandFn"]
    end
    A -->|每帧| C --> D
```

这意味着：
- **无需** `MainThreadDispatcher`
- **无需** 跨线程日志（直接调用 `UtilityFunctions::print`）
- **无需** tokio 运行时
- 无 `bind_mut` 死锁风险
- 所有 `cmd_*` 函数可以直接调用 Godot API

### Streamable HTTP

采用 JSON-RPC 2.0 作为协议层，通过 Server-Sent Events (SSE) 实现流式结果推送，兼容 MCP Streamable HTTP 传输规范。

### 函数指针路由

`HandlerRegistry` 维护一个从工具名到 `CommandFn` 函数指针的映射表，调用时按命中的工具名直接跳转，无反射开销。

## 编辑器插件生命周期

```mermaid
stateDiagram-v2
    [*] --> EnterTree: Godot 加载插件
    EnterTree --> Initializing: _enter_tree()
    
    state Initializing {
        [*] --> ReadVersion: 读取引擎版本 + 插件版本
        ReadVersion --> RegisterTools: register_itools()
        RegisterTools --> ReadPort: 读取端口
        ReadPort --> StartServer: http_server_.start()
        StartServer --> ConnectSignal: connect process_frame
        ConnectSignal --> [*]
    }
    
    Initializing --> Running
    Running --> Processing: process_frame 信号
    
    state Processing {
        [*] --> PollHTTP: http_server_.poll()
        PollHTTP --> [*]
    }
    
    Processing --> Exiting: 编辑器卸载
    Exiting --> [*]: _exit_tree() 清理
```

## 命令路由链路

完整的工具调用链路：

```
客户端 HTTP POST /mcp {"method":"tools/call","params":{"name":"get_node_position",...}}
 → HttpServer::handle_post()
   → 验证协议版本 / Content-Type / Accept / Origin
   → 解析 JSON-RPC 2.0 消息
 → McpHandler::handle_tools_call()
   → HandlerRegistry::find("get_node_position") → CommandFn
   → 主线程同步执行 CommandFn(args)
   → 包装返回值 → HTTP 200 + JSON-RPC Response
```

## 目录结构

```
extensions/src/
├── register_types.cpp       # GDExtension 入口（符号: gdext_rust_init）
├── editor_plugin.cpp/.hpp   # EditorPlugin 组装者
├── logging.hpp              # 日志工具
├── sdk/
│   ├── mcp_tool_definition.cpp/.hpp  # SDK 基类（GDScript 可继承）
│   └── mcp_tool_registry.cpp/.hpp    # 工具注册中心（单例）
├── server/
│   ├── ipc/http_server.cpp/.hpp      # HTTP 服务器
│   ├── mcp/mcp_handler.cpp/.hpp      # MCP 会话管理
│   └── registry/handler_registry.cpp/.hpp  # 工具注册表
├── built_in/
│   ├── cmd_info.cpp         # godot_info（连接状态+环境信息）
│   ├── cmd_meta_tools.cpp   # 渐进式披露 meta-tools（4）
│   ├── cmd_utils.cpp/.hpp   # 工具函数
│   ├── node.cpp             # 节点操作（21）
│   ├── property.cpp         # 2D 属性读写（21）
│   ├── property_3d.cpp      # 3D 属性读写（6）
│   ├── scene.cpp            # 场景文件/标签页操作（16）
│   ├── script_gd.cpp        # GDScript 命令（5）
│   ├── script_cs.cpp        # C# 命令（6）
│   ├── script_helpers.cpp   # call_method、get/set_variable（3）
│   ├── collision.cpp        # 碰撞体创建（2）
│   ├── find.cpp             # 节点搜索（4）
│   ├── search.cpp           # 文件搜索/替换（3）
│   ├── undo.cpp             # undo/redo（2）
    ├── editor_control.cpp   # 播放/停止、刷新（7）
    ├── project_settings.cpp      # 项目设置（7）
    ├── project_settings_ext.cpp  # 显示/物理/渲染设置（10）
    ├── input_map.cpp        # 输入映射（4）
    └── plugin_management.cpp     # 插件管理（2）
```

## 数据流

### 撤销支持

```mermaid
sequenceDiagram
    participant AI as AI 客户端
    participant Cmd as CommandFn
    participant EU as EditorUndoRedoManager
    participant Node as Godot 节点

    AI->>Cmd: set_property(position, ...)
    Cmd->>EU: create_action("Set Position")
    Cmd->>Node: 获取旧值
    Cmd->>EU: add_do_property(node, "position", newVal)
    Cmd->>EU: add_undo_property(node, "position", oldVal)
    Cmd->>EU: commit_action()
    EU-->>Cmd: 操作完成
    Cmd-->>AI: {success: true}
```
