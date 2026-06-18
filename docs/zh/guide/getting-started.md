# 快速入门

## 系统要求

- **Godot 4.6+**（编辑器，非导出模板）
- **支持 MCP Streamable HTTP 的 AI 客户端**（Claude Code、Cline、Continue、Cursor、opencode、Roo Code 等）

## 安装

### 1. 下载插件

从 [Releases](https://github.com/JessPig/GodotMCP-GDExtension/releases) 页面下载最新的 `addons.zip`。

### 2. 解压到项目目录

将 zip 解压到 Godot 项目根目录，插件文件结构如下：

```
your_project/
addons/
  godot_mcp/
    plugin.cfg
    godot_mcp.gdextension
    bin/
      godot_mcp_gdext.dll    # Windows
      libgodot_mcp_gdext.so  # Linux
      libgodot_mcp_gdext.dylib  # macOS
```

### 3. 启用插件

1. 在 Godot 4.6+ 中打开你的项目
2. 进入 **项目 > 项目设置 > 插件**
3. 找到 **Godot MCP** 并将状态设为 **启用**

### 4. 验证是否生效

查看 Godot 编辑器输出控制台是否有：

```
GodotMCP: HTTP server started on port 9600
```

或使用 curl 测试：

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_info","arguments":{}}}'
```

预期响应：

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{
      "type": "text",
      "text": "{\"success\":true,\"data\":{\"connection\":{\"status\":\"ok\"},\"engine\":{...}}}"
    }]
  }
}
```

## 配置 AI 客户端

参见[客户端设置](/zh/guide/client-setup)了解如何配置 opencode、Cursor、Claude Code、Cline 等 AI 工具。

## 下一步

- 阅读[工具概述](/zh/guide/tools-overview)了解可用的功能
- 浏览[工具参考](/zh/reference/meta-tools)获取详细的工具参数
- 查看[常见问题](/zh/guide/faq)解决常见问题