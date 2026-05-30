# LSP 验证客户端

> 使用 Godot 内置的 `StreamPeerTCP` 连接 GDScript LSP 服务器进行语法验证。

## 验证流程

1. **连接**：`StreamPeerTCP::connect("127.0.0.1", 6005)`，超时 2 秒
2. **初始化**：发送 `initialize` 请求并等待响应
3. **验证**：发送 `textDocument/didOpen`（含源码），读取 `publishDiagnostics`
4. **清理**：发送 `shutdown` + `exit`

## 关键细节

- **传输**：`StreamPeerTCP`（Godot 内置 TCP 客户端）
- **LSP 帧格式**：HTTP 风格 `Content-Length: xxx\r\n\r\n<body>`
- **端口**：`127.0.0.1:6005`（Godot LSP 默认端口）
- **同步**：所有 I/O 在主线程中阻塞执行

## 注意事项

- Godot 编辑器设置中必须启用 Language Server（`编辑器 → 网络 → Language Server → 启用`）
- 如果 LSP 服务器未运行，连接会超时并返回错误，此时降级为基本编译检查
- 只支持 GDScript 验证（不验证 C#）
