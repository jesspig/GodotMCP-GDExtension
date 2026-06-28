# 技术细节

## 线程模型

GodotMCP 采用**纯主线程**、单进程架构。没有工作线程，没有线程池，没有互斥锁，也没有无锁数据结构。

### 轮询循环

McpEditorPlugin::_process(delta) 每编辑器帧运行，驱动两个子系统：

`cpp
void McpEditorPlugin::_process(double delta) {
    http_server_.poll();      // 接受连接、解析 HTTP、分派到 MCP handler
    runtime_bridge_.poll();   // 从游戏进程桥接读取 TCP 数据
}
`

这意味着：
- **所有工具执行** 都在主线程的 _process 中发生
- **无需担心线程安全** — 可自由调用 Godot API
- **无需 MainThreadDispatcher** 模式
- **阻塞型工具**（例如带超时的 wait_for_bridge）使用帧计数器方式：
  `cpp
  if (elapsed_frames_++ > timeout_frames) { return ToolResult::err("TIMEOUT", "..."); }
  `

## 协议细节

### MCP Streamable HTTP 2026-07-28

服务器实现了 MCP Streamable HTTP 传输（2026-07-28 修订版）。

| 方面 | 详情 |
|--------|--------|
| 端点 | POST http://127.0.0.1:9600/mcp |
| 端口配置 | GODOT_MCP_HTTP_PORT 环境变量或 ProjectSetting godot_mcp/http_port（默认：9600） |
| 主机配置 | GODOT_MCP_HTTP_HOST 环境变量（默认：127.0.0.1） |
| 协议 | JSON-RPC 2.0 |
| 会话 | **无** — 无状态，无会话 ID |
| GET 端点 | **SSE 流式** — 支持 GET、POST、OPTIONS |
| SSE | 内联在 POST 响应体中 |

### HTTP 头

请求必须包含：
- Content-Type: application/json
- Accept: application/json, text/event-stream

服务器会校验：
- Mcp-Method 头与请求体中的 method 匹配
- Mcp-Name 头与工具调用的 params.name 匹配（如果存在）
- Origin 头用于 CORS

### 错误响应格式

`json
{
    "jsonrpc": "2.0",
    "id": 1,
    "error": {
        "code": -32603,
        "message": "Tool execution failed",
        "data": {
            "success": false,
            "error": {
                "code": "TOOL_NOT_FOUND",
                "message": "Tool 'nonexistent' not found"
            }
        }
    }
}
`

### SSE 事件格式

对于流式响应，事件格式如下：

`
event: message
data: {"jsonrpc":"2.0","id":1,"result":{"content":[{"type":"text","text":"..."}]}}

event: error
data: {"jsonrpc":"2.0","id":1,"error":{"code":-32603,"message":"..."}}
`

## 桥接协议

编辑器通过 TCP 端口 9601 连接到游戏进程。

| 方面 | 详情 |
|--------|--------|
| 端口配置 | GODOT_MCP_BRIDGE_PORT 环境变量（默认：9601） |
| 方向 | 编辑器（客户端）→ 游戏（服务端） |
| 帧界定 | 换行符分隔的 JSON（\n） |
| 缓冲区 | 每条消息最大 64KB |
| 自动检测 | EditorInterface::is_playing_scene() 用于游戏启动/停止 |

### 消息流

`
Editor → Game: {"cmd":"get_scene_tree","args":{},"id":1}
Game → Editor: {"ok":true,"data":{...},"id":1}
`

## 配置参考

### 环境变量

| 变量 | 默认值 | 描述 |
|----------|---------|-------------|
| GODOT_MCP_HTTP_PORT | 9600 | HTTP 服务器端口 |
| GODOT_MCP_HTTP_HOST | 127.0.0.1 | HTTP 服务器绑定地址 |
| GODOT_MCP_BRIDGE_PORT | 9601 | 运行时桥接 TCP 端口 |

### ProjectSettings

| 键 | 类型 | 默认值 | 描述 |
|-----|------|---------|-------------|
| godot_mcp/http_port | int | 9600 | HTTP 服务器端口（覆盖环境变量） |

## 启动时序

1. Godot 加载 GDExtension → 调用 gdext_mcp_init()
2. McpEditorPlugin 注册为 EditorPlugin
3. _enter_tree() 触发：
   - 创建 HandlerRegistry 并通过 X-macro 注册所有 164 个内置工具
   - 创建 McpToolRegistry 单例供 SDK 访问
   - 读取端口配置
   - 在配置的端口上启动 HttpServer
4. _process() 开始每帧轮询 HTTP 和桥接连接
5. 编辑器退出时，_exit_tree() 关闭 HTTP 服务器并清理资源