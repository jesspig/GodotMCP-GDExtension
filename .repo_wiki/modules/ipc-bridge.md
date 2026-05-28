# IPC 桥接（WebSocket）

> 连接 `godot-mcp-server`（Python）和 `godot_mcp_gdext`（C++）的通信桥梁。

## 同步 WebSocket

```mermaid
sequenceDiagram
    participant Server as godot-mcp-server (Python)
    participant GDExt as godot_mcp_gdext (C++)
    
    Note over Server,GDExt: 启动阶段
    
    GDExt->>GDExt: TCPServer::listen(9500, "127.0.0.1")
    GDExt->>GDExt: 每帧 poll() 检查新连接
    GDExt->>GDExt: 接受 TCP → WebSocketPeer::accept_stream()
    GDExt-->>Server: godot_ready 通知
    
    Note over Server,GDExt: 工具调用
    
    Server->>GDExt: send(IpcRequest JSON)
    GDExt->>GDExt: poll() → 读取 → handle_packet()
    GDExt->>GDExt: HandlerRegistry::find() → CommandFn()
    GDExt-->>Server: IpcResponse JSON
```

C++ 版本使用 Godot 4.6 内置的 `TCPServer` + `WebSocketPeer`：

- **同步架构**：所有事件通过 `WsServer::poll()` 在 Godot 主线程上处理
- **无心跳**（当前版本）：依赖 TCP 自身连接管理
- **单线程**：无需 tokio 运行时

### C++ 消息处理

```cpp
// ws_server.cpp  handle_packet()
void WsServer::handle_packet(int peer_id, Ref<WebSocketPeer> peer, const String &text) {
    // 1. 解析 JSON
    Ref<JSON> json;
    json.instantiate();
    Error err = json->parse(text);
    if (err != OK) { /* 返回 INVALID_REQUEST 错误 */ }
    
    const Dictionary req = json->get_data();
    const String method = req["method"];
    
    // 2. 验证 method
    if (method != "tool_call") { /* 返回错误 */ }
    
    // 3. 提取 tool + args
    const Dictionary params = req["params"];
    const String tool = params["tool"];
    const Dictionary args = params["args"];
    
    // 4. 查找并执行
    const CommandFn *fn = registry_->find(tool);
    if (!fn) { /* UNKNOWN_TOOL 错误 */ }
    
    Dictionary result = (*fn)(args);  // 同步执行
    
    // 5. 返回响应
    Dictionary flat;
    if (result.has("error")) {
        flat["status"] = "error";
        flat["message"] = result["error"];
    } else {
        flat["status"] = "success";
        flat["data"] = result;
    }
    peer->send_text(JSON::stringify(flat));
}
```

## 线路格式

### 请求（Server → GDExt）

```json
{
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "method": "tool_call",
    "params": {
        "tool": "get_node_position",
        "args": {"node_path": "Player"}
    }
}
```

### 响应成功（GDExt → Server）

```json
{"id": "uuid", "status": "success", "data": {"x": 100.0, "y": 200.0}}
```

### 响应错误（GDExt → Server）

```json
{"id": "uuid", "status": "error", "code": "TOOL_FAILED", "message": "..."}
```

### 通知（GDExt → Server）

```json
{"type": "notification", "event": "godot_ready", "data": {"engine_version": "4.6.0", "plugin_version": "0.1.5-dev.1"}}
```

## 类型定义

### C++（`protocol/ipc_types.hpp`）

```cpp
constexpr const char *kStatusSuccess = "success";
constexpr const char *kStatusError = "error";
constexpr const char *kErrCodeInvalidRequest = "INVALID_REQUEST";
constexpr const char *kErrCodeUnknownTool = "UNKNOWN_TOOL";
constexpr const char *kErrCodeToolFailed = "TOOL_FAILED";
constexpr const char *kErrCodeInternal = "INTERNAL_ERROR";
```

- `make_success_result(data)` / `make_error_result(code, message)`
- `make_response(id, result)` / `make_notification(event, data)`

### Python（`server/src/godot_mcp_server/protocol.py`）

```python
class IpcRequest(BaseModel): id: str; method: str; params: dict
class IpcResponse(BaseModel): id: str; status: str = "ok"; data: Any; code: int; message: str
class IpcNotification(BaseModel): type: str; event: str; data: dict
class ToolCallParams(BaseModel): tool: str; args: dict = {}
```

## 错误码

| 错误场景 | C++ 错误码 |
|----------|-----------|
| 无效 JSON | `INVALID_REQUEST` |
| 未知 method | `INVALID_REQUEST` |
| 未知工具 | `UNKNOWN_TOOL` |
| 执行失败 | `TOOL_FAILED` |
| 内部错误 | `INTERNAL_ERROR` |
