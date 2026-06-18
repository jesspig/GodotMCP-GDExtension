# 配置

## 环境变量

| 变量 | 默认值 | 描述 |
|----------|---------|-------------|
| `GODOT_MCP_HTTP_PORT` | `9600` | HTTP 服务器端口 |
| `GODOT_MCP_HTTP_HOST` | `127.0.0.1` | HTTP 服务器绑定地址 |
| `GODOT_MCP_BRIDGE_PORT` | `9601` | 运行时桥接 TCP 端口 |

在系统环境变量中设置，或在启动 Godot 时传入：

```bash
# Windows (命令提示符)
set GODOT_MCP_HTTP_PORT=9600
godot --editor

# Windows (PowerShell)
$env:GODOT_MCP_HTTP_PORT = 9600
godot --editor

# Linux/macOS
GODOT_MCP_HTTP_PORT=9600 godot --editor
```

## 项目设置

HTTP 端口也可以通过项目设置来配置：

| 键 | 类型 | 默认值 | 描述 |
|-----|------|---------|-------------|
| `godot_mcp/http_port` | int | `9600` | HTTP 服务器端口（覆盖环境变量） |

修改方法：

1. 进入 **项目 > 项目设置**
2. 搜索 `godot_mcp`
3. 设置所需的端口值
4. 重启编辑器

## 端口冲突解决

如果端口 9600 已被占用：

1. 查看占用端口的程序：`netstat -ano | findstr :9600`
2. 通过环境变量或项目设置设置不同的端口
3. 更新 AI 客户端的 MCP 配置以匹配
4. 重启 Godot

## 聚合项目设置工具

GodotMCP 提供 10 个聚合设置工具（5 个 get/set 对），将相关的项目设置属性捆绑在一起：

| 分类 | 获取工具 | 设置工具 | 涉及的属性 |
|----------|----------|----------|-------------------|
| 显示 | `get_display_settings` | `set_display_settings` | 窗口大小、模式、垂直同步、MSAA、缩放 |
| 项目信息 | `get_project_info_settings` | `set_project_info_settings` | 名称、描述、版本、公司 |
| 物理 | `get_physics_settings` | `set_physics_settings` | FPS、重力、阻尼、层 |
| 渲染 | `get_rendering_settings` | `set_rendering_settings` | 渲染器、质量、阴影、反射 |
| 层名称 | `get_layer_names_settings` | `set_layer_names_settings` | 2D/3D 物理和渲染层名称 |

这些工具可避免为常见配置任务发起 20+ 次独立的 `set_setting` 调用。