# 设计决策

> ADR 风格的关键架构决策记录。

## ADR-001: 双进程架构（Server + GDExtension）

**状态**：已接受  
**日期**：初始项目设置  
**背景**：GDExtension CDyLib 允许我们加载到 Godot 编辑器中访问原生 API，但不能作为独立的 MCP 服务器。同时，MCP 服务器（一个标准的 Rust 二进制文件）可以处理 stdio、rmcp 和 AI 客户端交互，但不能调用 Godot API。  
**决策**：将系统拆分为两个进程，通过 WebSocket 连接。  
**后果**：
- 正面：清晰的关注点分离；可以分别开发、测试两个组件
- 正面：MCP 服务器可以独立启动/重启，与编辑器生命周期分离
- 负面：WebSocket IPC 增加了延迟；引入协议版本同步问题
- 负面：需要处理两进程的启动顺序和连接管理

## ADR-002: WebSocket 而非 Unix Domain Socket / Named Pipe

**状态**：已接受  
**背景**：需要跨平台 IPC 解决方案。Unix 域套接字在 Windows 上不好用（Named Pipes 更合适但在 Rust 中比较麻烦）。  
**决策**：使用 WebSocket（ws://127.0.0.1:9500），跨平台可靠且在 Rust 生态系统中支持良好。  
**后果**：
- 正面：`tokio-tungstenite` 在 Windows、macOS、Linux 上开箱即用
- 正面：也容易用 wscat 等工具调试
- 负面：如果用户同时打开多个 Godot 实例，有端口冲突风险

## ADR-003: `process_frame` 而非 `EditorPlugin::_process()` 用于泵调度

**状态**：已接受  
**背景**：`EditorPlugin` trait 提供了 `_process(&mut self, delta)` 方法，但我们发现如果在该上下文中调用某些 Godot API（如 `EditorInterface`），会遇到 `Gd::bind_mut()` 死锁。  
**决策**：使用 `Callable::from_fn` 连接 `SceneTree::process_frame` 信号来泵 dispatcher 和日志队列，而不是重写 `_process()`。  
**后果**：
- 正面：完全避免 `bind_mut` 死锁
- 正面：`McpEditorPlugin::_process()` 被有意留空——清楚表明不在此处运行逻辑
- 负面：增加了一层间接

## ADR-004: 注册静态所有工具，不在运行时从 gdext 查询

**状态**：已接受  
**背景**：MCP 协议要求服务器在 `list_request_tools` 响应中提供工具列表，包括完整的 JSON Schema。  
**决策**：在服务器侧静态声明所有 99 个工具的 Schema。gdext 侧有第二个注册表，但仅用于发现工具名称并在路由时匹配。  
**后果**：
- 正面：`list_request_tools` 可在没有 Godot 编辑器运行时响应（服务器可独立启动）
- 正面：Schema 的定义靠近其消费端（服务器）
- 负面：两次注册（服务器和 gdext）必须保持同步——添加/删除工具时需要在两侧更新
- 负面：测试依赖 `total == 99` 的硬编码断言

## ADR-005: 使用 CMake + Corrosion 构建（非独立 Cargo）

**状态**：已接受  
**背景**：项目需要构建一个 Rust CDyLib（`godot_mcp_gdext.dll`）和一个 Rust 二进制（`godot-mcp-server.exe`）。构建后还涉及复制 DLL、生成 `plugin.cfg`、打包 `addons.zip` 等操作。  
**决策**：使用 CMake 作为顶级构建系统，通过 Corrosion 集成 Rust。  
**后果**：
- 正面：统一的构建入口——`cmake --build`
- 正面：CMake 处理跨平台任务（进程终止、文件操作、CPack 打包）
- 负面：需要 CMake + Corrosion——不是纯粹的 Cargo 工作流
- 负面：`build.py` 包装器是必要的，但不是必需的

## ADR-006: C# Solution 直接生成（不启动第二个 Godot 进程）

**状态**：已接受  
**背景**：Godot 的 `--generate-mono-solution` 标志启动一个新的编辑器进程，与我们的 gdext 插件竞争 WebSocket 端口 9500。  
**决策**：直接在 Rust 中生成 `.sln` 和 `.csproj` 文件，无需启动编辑器进程。  
**后果**：
- 正面：无端口冲突
- 正面：速度更快（无进程开销）
- 负面：需要维护与 Godot 4.6 .NET SDK 格式兼容的模板
- 负面：`.sln` 和 `.csproj` 生成的逻辑需要跟踪 Godot .NET SDK 版本的变化

## ADR-007: 跨线程日志使用 mpsc + eprintln 镜像

**状态**：已接受  
**背景**：Godot 的 `godot_print!` 宏如果从非主线程调用会发生未定义行为（通常崩溃）。  
**决策**：tokio 工作线程调用 `log_info/log_warn/log_error` → 消息进入 mpsc 通道 + `eprintln!` 到终端。主线程从 `process_frame` 信号泵中转发到 `godot_print!`。  
**后果**：
- 正面：工作线程上的日志调用可靠且立即
- 正面：日志最终出现在 Godot 控制台和客户端终端中
- 负面：Godot 控制台中的日志最多延迟一帧

## ADR-008: `call_method` 使用 `Object::call()`（不通过 Deferred/Callable）

**状态**：已接受  
**背景**：需要一个通用的方式来调用节点上的任何方法。  
**决策**：使用 `node.call(method, &args)` 直接调用。  
**后果**：
- 正面：支持任意方法签名
- 正面：立即执行（gdext 端已经运行在主线程上）
- 负面：如果方法预期参数与实际传递的类型不匹配，可能在运行时失败
