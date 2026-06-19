# 架构决策与技术约束

## 1. 核心架构约束

### 1.1 纯主线程模型

`McpEditorPlugin::_process()` 驱动所有 I/O：HTTP 轮询 + WebSocket 轮询 + 桥接轮询 + UI 更新。

**Godot 源码验证**：
- `TCPServer`、`StreamPeerTCP` 无线程安全保证（源码无 `_THREAD_SAFE_METHOD_` 宏）
- 设计为主线程轮询（`poll()` 非阻塞，timeout=0）
- `HashingContext` 无线程安全保证，一次只能有一个活跃哈希上下文

**决策**：M1（工作线程池）暂缓。当前 153 工具 + 30req/s 限流下 CPU 开销 <1%。

### 1.2 进程内 GDExtension

所有工具通过 `EditorInterface` 直接调用 Godot API，零序列化开销。

**GDExtension 可用 API 汇总**（Godot 源码验证）：

| API | 文件 | 签名 |
|-----|------|------|
| `String::sha1_buffer()` | `core/string/ustring.h:603` | `Vector<uint8_t> sha1_buffer() const` |
| `String::to_utf8_buffer()` | `core/string/ustring.h:670` | `Vector<uint8_t> to_utf8_buffer() const` |
| `String::utf8()` | `core/string/ustring.h:540` | `static String utf8(const char*, int)` |
| `Marshalls::raw_to_base64()` | `core/core_bind.h:435` | `String raw_to_base64(const Vector<uint8_t>&)` |
| `Time::get_ticks_msec()` | `core/os/time.h:77` | `uint64_t get_ticks_msec() const` |
| `OS::get_process_id()` | `core/core_bind.h:231` | `int get_process_id() const` |
| `Performance::get_monitor()` | `main/performance.h` | `double get_monitor(Monitor) const` |
| `EditorUndoRedoManager::get_history_undo_redo()` | `editor/editor_undo_redo_manager.h` | `UndoRedo* get_history_undo_redo(int)` |
| `UndoRedo::get_current_action_name()` | `core/object/undo_redo.h` | `String get_current_action_name() const` |
| `Engine::is_editor_hint()` | `core/config/engine.h:172` | `bool is_editor_hint() const` |

### 1.3 MCP 协议

- 协议版本：MCP 2026-07-28（最新），向下兼容 2025-11-25 和 2025-03-26
- 传输层：Streamable HTTP（`POST /mcp` + `GET /mcp` SSE）
- 无 session：每个请求独立处理
- 工具发现：渐进式（`tools/list` 返回元工具 → `list_categories` → `get_tools_in_category`）

## 2. 跨平台策略

### 2.1 决策：不需要条件编译 socket 代码

Godot 的 `TCPServer`、`StreamPeerTCP` 已封装所有平台差异：
- Windows: WinSock2 内部封装
- Linux: POSIX sockets 内部封装
- macOS: BSD sockets 内部封装

**需要条件编译的唯一场景**：CMake 构建系统（已由 `CMakePresets.json` 处理）。

### 2.2 WebSocket SHA-1 / Base64 — 使用 Godot 内置

| 功能 | 方案 | 依赖 |
|------|------|------|
| SHA-1 | `String::sha1_buffer()` | 零外部依赖 |
| Base64 | `Marshalls::get_singleton()->raw_to_base64()` | 零外部依赖 |

**无需** TinySHA1、OpenSSL 或内联 base64 实现。

### 2.3 端序处理

WebSocket 帧的 16-bit 和 64-bit 长度字段使用网络字节序（big-endian）。

```cpp
// 使用 Godot 内置端序工具（core/typedefs.h）
#include <godot_cpp/core/defs.hpp>

// encode_uint16 / encode_uint32 / decode_uint16 / decode_uint32
// 默认 big-endian（网络序）
```

## 3. C++11-17 现代化特性

### 3.1 特性应用清单

| 特性 | 版本 | 应用场景 | 示例 |
|------|------|---------|------|
| `[[nodiscard]]` | C++17 | 所有返回错误码或可空结果的函数 | `[[nodiscard]] std::optional<int> decode(...)` |
| `constexpr` | C++11 | 编译期常量 | `static constexpr uint16_t kDefaultPort = 9600;` |
| `enum class` | C++11 | 类型安全枚举 | `enum class WsOpcode : uint8_t { ... };` |
| `std::optional` | C++17 | 可空返回值（替代 error code + out param） | `std::optional<int> try_parse_frame(...)` |
| 结构化绑定 | C++17 | HashMap 遍历、pair 解包 | `for (auto &[name, info] : tool_info_)` |
| 移动语义 | C++11 | PackedByteArray 传递、大对象转移 | `PackedByteArray encode(WsOpcode, ...)` |
| `noexcept` | C++11 | 纯 getter 方法 | `int port() const noexcept { return port_; }` |
| `if constexpr` | C++17 | 编译期分支（日志级别） | `if constexpr (kDebugLogging) { ... }` |
| `static_assert` | C++11 | 编译期大小检查 | `static_assert(kMaxMessageSize <= 65536);` |
| `std::chrono` | C++11 | 内部耗时测量 | `auto start = steady_clock::now();` |
| `= default` | C++11 | 默认构造/析构 | `WebSocketCodec() = default;` |
| `= delete` | C++11 | 禁止拷贝 | `WebSocketCodec(const WebSocketCodec&) = delete;` |
| `using` 别名 | C++11 | 类型别名（替代 typedef） | `using EventCallback = std::function<void(...)>;` |
| 智能指针 | C++11 | Godot Ref<T> 已是智能指针 | `Ref<StreamPeerTCP> tcp_;` |

### 3.2 不使用的特性（超出 C++17 或不适用）

| 特性 | 原因 |
|------|------|
| `std::string_view` | Godot 使用自己的 `String` 类型，不使用 STL string |
| `std::variant` | Godot 使用 `Variant` 类型系统 |
| `std::byte` | Godot 使用 `uint8_t`（`PackedByteArray`） |
| `concepts` | C++20，超出范围 |
| `coroutines` | C++20，超出范围 |
| `std::format` | C++20，超出范围 |

### 3.3 命名约定

遵循 Godot 引擎命名规范：
- 类名：`PascalCase`（`WebSocketCodec`、`GameBridgeClient`）
- 方法名：`snake_case`（`connect_to_editor()`、`handle_ws_handshake()`）
- 成员变量：`snake_case_` 尾下划线（`state_`、`read_buf_`）
- 常量：`kPascalCase` 前缀 k（`kDefaultPort`、`kMaxMessageSize`）
- 枚举值：`PascalCase`（`WsOpcode::Text`、`BridgeState::Connected`）
- 回调/信号：`snake_case`（`_on_confirm_allow`）

## 4. 文件组织

### 4.1 新增文件清单

```
extensions/src/
├── server/
│   ├── ipc/
│   │   └── websocket.hpp/cpp          ← Sprint 3: WS 帧编解码
│   └── mcp/
│       ├── pending_operation.hpp       ← Sprint 1: 破坏性操作队列
│       └── rollback_operation.hpp/cpp  ← Sprint 2: 回滚工具
├── ui/
│   └── mcp_confirm_dialog.hpp/cpp      ← Sprint 1: 确认对话框
└── game/
    └── print_capture.gd                ← Sprint 3: 可选 GDScript autoload
```

### 4.2 删除文件清单

```
extensions/src/runtime/
├── bridge.hpp      ← Sprint 3: 被 HttpServer WS 层替代
└── bridge.cpp      ← Sprint 3: 同上
```

### 4.3 重写文件清单

```
extensions/src/runtime/
├── game_bridge.hpp  ← Sprint 3: TCP 服务器 → WS 客户端
└── game_bridge.cpp  ← Sprint 3: 同上
```
