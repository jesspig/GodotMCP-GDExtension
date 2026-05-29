# 设计决策

> ADR 风格的关键架构决策记录。

## ADR-001: 双进程架构（Server + GDExtension）

**状态**：已接受  
**日期**：初始项目设置  
**背景**：GDExtension CDyLib 允许我们加载到 Godot 编辑器中访问原生 API，但不能作为独立的 MCP 服务器。同时，MCP 服务器需要处理 stdio 和 AI 客户端交互，但不能调用 Godot API。  
**决策**：将系统拆分为两个进程，通过 WebSocket 连接。  
**后果**：
- 正面：清晰的关注点分离；可以分别开发、测试两个组件
- 正面：MCP 服务器可以独立启动/重启，与编辑器生命周期分离
- 负面：WebSocket IPC 增加了延迟；引入协议版本同步问题
- 负面：需要处理两进程的启动顺序和连接管理

## ADR-002: WebSocket 而非 Unix Domain Socket / Named Pipe

**状态**：已接受  
**背景**：需要跨平台 IPC 解决方案。Unix 域套接字在 Windows 上不好用（Named Pipes 更合适但复杂）。  
**决策**：使用 WebSocket（`ws://127.0.0.1:9500`），利用 Godot 内置 `TCPServer` + `WebSocketPeer` 实现。  
**后果**：
- 正面：Godot 内置 WebSocket 支持在 Windows、macOS、Linux 上开箱即用
- 正面：也容易用 wscat 等工具调试
- 正面：无第三方依赖（无 tokio-tungstenite）
- 负面：如果用户同时打开多个 Godot 实例，有端口冲突风险

## ADR-003: `process_frame` 而非 `EditorPlugin::_process()` 用于驱动

**状态**：已接受  
**背景**：`EditorPlugin::_process()` 在场景播放时停止触发，不适合需要持续轮询的 WebSocket 服务器。  
**决策**：使用 `SceneTree::process_frame` 信号来驱动 `WsServer::poll()`，而不是重写 `_process()`。  
**后果**：
- 正面：`process_frame` 在场景播放时继续触发，确保实时工具正常工作
- 正面：`McpEditorPlugin::_process()` 被有意留空——清楚表明不在此处运行逻辑
- 负面：增加了一层间接

## ADR-004: 注册静态所有工具，不在运行时从 gdext 查询

**状态**：已接受  
**背景**：MCP 协议要求服务器在 `list_request_tools` 响应中提供工具列表，包括完整的 JSON Schema。  
**决策**：在服务器侧静态声明所有 125 个工具的 Schema。gdext 侧有第二个注册表，但仅用于发现工具名称并在路由时匹配。  
**后果**：
- 正面：`list_request_tools` 可在没有 Godot 编辑器运行时响应（服务器可独立启动）
- 正面：Schema 的定义靠近其消费端（服务器）
- 负面：两次注册（服务器和 gdext）必须保持同步——添加/删除工具时需要在两侧更新
- 负面：测试依赖 `total == 125` 的硬编码断言

## ADR-005: 使用 CMake 构建（GDExtension + 服务器）

**状态**：已接受  
**背景**：godot-cpp 通过 FetchContent 拉取，Cython --embed 编译服务器。  
**决策**：使用 CMake 作为顶级构建系统，通过 `add_subdirectory(extensions/gdext)` 构建 C++ GDExtension + Cython --embed 编译服务器。  
**后果**：
- 正面：统一的构建入口——`cmake --build`
- 正面：CMake 处理跨平台任务（进程终止、文件操作、CPack 打包）
- 正面：`build.py` 是便捷包装，但 CMake 可直接使用

## ADR-006: C# Solution 直接生成（不启动第二个 Godot 进程）

**状态**：已接受  
**背景**：Godot 的 `--generate-mono-solution` 标志启动一个新的编辑器进程，与我们的 gdext 插件竞争 WebSocket 端口 9500。  
**决策**：直接在 gdext 中生成 `.sln` 和 `.csproj` 文件，无需启动编辑器进程。  
**后果**：
- 正面：无端口冲突
- 正面：速度更快（无进程开销）
- 负面：需要维护与 Godot 4.6 .NET SDK 格式兼容的模板
- 负面：`.sln` 和 `.csproj` 生成的逻辑需要跟踪 Godot .NET SDK 版本的变化

## ADR-007: 放弃跨线程日志方案，采用直接 GDExtension API

**状态**：已接受  
**背景**：C++ 版本所有代码在主线程运行，没有跨线程日志需求。  
**决策**：直接调用 `UtilityFunctions::print/push_warning/push_error`，28 行实现覆盖所有日志需求。  
**后果**：
- 正面：日志即时输出，无延迟
- 正面：28 行代码 vs Rust 版本的 137 行

## ADR-008: `call_method` 使用 `Object::call()`（不通过 Deferred/Callable）

**状态**：已接受  
**背景**：需要一个通用的方式来调用节点上的任何方法。  
**决策**：使用 `node.call(method, &args)` 直接调用。  
**后果**：
- 正面：支持任意方法签名
- 正面：立即执行（gdext 端已经运行在主线程上）
- 负面：如果方法预期参数与实际传递的类型不匹配，可能在运行时失败

## ADR-009: C++ GDExtension 重写（取代 Rust 实现）

**状态**：已接受  
**日期**：2025-2026  
**背景**：Rust 的 `gdext` crate（v0.5）存在严重的线程复杂性问题——需要 `MainThreadDispatcher`、MPSC 日志通道、tokio 运行时等复杂基础设施。约 50% 的代码用于处理跨线程通信而不是实际逻辑。  
**决策**：使用 C++ 和 `godot-cpp 10.0.0-rc1` 重写 GDExtension。利用 Godot C++ API 在主线程直接运行，消除所有跨线程基础设施。  
**后果**：
- 正面：消除整个 dispatcher 层（~300 行 Rust 删除）
- 正面：日志直接调用 `UtilityFunctions::print`（28 行 vs 137 行）
- 正面：WebSocket 使用 Godot 内置 `TCPServer` + `WebSocketPeer`（无 tokio 依赖）
- 正面：JSON↔Variant 转换使用 Godot 原生 `Dictionary`/`JSON`（无 serde 依赖）
- 正面：构建系统简化——无 Corrosion，无 tokio
- 正面：编译速度显著提升（C++ 比 Rust gdext 编译快得多）
- 正面：Rust 遗留代码（`crates/`）已被完全移除——Cargo workspace、`cargo test`、`Cargo.lock` 等均不再存在
- 负面：C++ 缺乏 Rust 的所有权和生命周期保证

## ADR-010: 新增 MCP Streamable HTTP 传输

**状态**：已接受  
**日期**：2026  
**背景**：Legacy WebSocket IPC 路径需要 Python 服务器中转（stdio → Python → WebSocket → C++），增加了延迟和部署复杂度。MCP 规范定义了 Streamable HTTP 传输，支持 AI 客户端直接连接。  
**决策**：在 gdext 中实现 `HttpServer`（HTTP + SSE）和 `McpHandler`（JSON-RPC 2.0 会话管理），监听 `:9600`，支持 AI 客户端直接通过 HTTP 连接。  
**后果**：
- 正面：消除 Python 中转层延迟（可选完全绕过 godot-mcp-server.exe）
- 正面：符合 MCP 规范的标准化传输
- 正面：支持 SSE 服务器推送事件
- 负面：两个端口需要管理（9500 legacy + 9600 HTTP）
- 负面：增加了 HTTP 解析和会话管理代码量

## ADR-011: `register_script_cs` 声明但未注册

**状态**：已接受  
**背景**：C# 脚本工具（`script_cs.cpp`，6 个工具）已实现并通过 `register_script_cs` 注册函数定义，但 `register_all_tools()` 中未调用。  
**决策**：保持声明但不调用，直到 C# 支持经过完整测试。  
**后果**：
- 正面：避免暴露未充分测试的工具
- 负面：Python 侧 registry.py 注册了这 6 个工具的 schema，但运行时不可用
- 负面：需要跟踪此状态以避免混淆
