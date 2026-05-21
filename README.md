# Godot MCP

Model Context Protocol 服务器，为 AI 助手提供 Godot 引擎的深度集成能力。

## 架构

### 两个进程

- `godot-mcp-server`: 独立的 MCP 服务器，通过 stdio 与 AI 客户端通信
- `godot_mcp_gdext`: GDExtension 插件，运行在 Godot 编辑器中

### 通信方式

两个进程通过 WebSocket 通信（`ws://127.0.0.1:9500`）

## 当前进度

✅ Cargo Workspace & core 协议类型
✅ GDExtension 基础框架 + EditorPlugin
✅ MCP Server 基础框架（rmcp stdio）
✅ WebSocket IPC 通信（ping/pong）
❌ Dock UI
❌ 完整工具集

## 构建

### 一键打包

```bash
python package_addons.py
```

自动完成：
1. 编译 `godot-mcp-gdext`（GDExtension 插件）
2. 编译 `godot-mcp-server`（MCP 服务端）
3. 复制库文件到插件目录
4. 打包 `addons.zip`

产物：
- `addons.zip` —— 解压到 Godot 项目根目录安装插件
- `target/debug/godot-mcp-server.exe` —— MCP 服务端可执行文件

## 使用

### Godot 编辑器

1. 解压 `addons.zip` 到 Godot 项目根目录
2. 打开 Godot → 项目设置 → 插件 → 启用「Godot MCP」
3. 控制台应出现 `[Godot MCP] Plugin loaded!`

### 配置 AI 客户端

以 OpenCode 为例，在 `~/.config/opencode/config.json` 中添加：

```json
{
    "mcpServers": {
        "godot-mcp": {
            "command": "path/to/godot-mcp-server.exe",
            "args": []
        }
    }
}
```

更多客户端配置参考 `.repo_wiki/guide/client-config.md`。

### 使用流程

```
1. 启动 Godot 编辑器（插件自动启动 WebSocket 在 9500 端口）
2. 在 AI 客户端中调用 MCP 工具即可
```

## 协议设计

完整的协议和设计文档请参见 `.repo_wiki/`。