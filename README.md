# Godot MCP

Godot引擎的Model Context Protocol (MCP)桥接方案。

## 项目结构

```
GodotMCP/
├── crates/
│   ├── core/          # 共享协议类型定义
│   ├── gdext/         # Godot GDExtension插件
│   └── server/        # MCP服务器
├── addons/
│   └── godot_mcp/     # Godot插件目录
└── .repo_wiki/        # 项目文档
```

## 当前状态

✅ **Phase 1 MVP - 基础架构已完成**
- Cargo workspace结构搭建完成
- Core crate：协议类型定义
- Server crate：基础MCP stdio服务器（支持ping工具）
- GDExtension crate：基础可加载插件
- 打包脚本：Python版本

## 快速开始

### 0. 一键打包插件

使用 Python 脚本一键编译和打包：

```bash
python package_addons.py
```

参数说明：
- `--no-build` - 跳过编译，直接打包
- `--source DIR` - 指定源目录（默认: addons）
- `--output FILE` - 指定输出文件（默认: addons.zip）

生成的 `addons.zip` 可以直接解压到 Godot 项目的根目录使用。

### 1. 编译项目

如果需要单独编译：

```powershell
# 编译所有crate
cargo build --workspace

# 或者单独编译
cargo build -p godot-mcp-core
cargo build -p godot-mcp-server
cargo build -p godot-mcp-gdext
```

### 2. 在Godot中安装插件

1. 将 `addons/godot_mcp/` 文件夹复制到你的Godot项目的 `addons/` 目录
2. 在Godot编辑器中：`项目` → `项目设置` → `插件`
3. 启用 "Godot MCP" 插件

### 3. 配置MCP客户端

参考 `.repo_wiki/guide/client-config.md` 中的配置说明。

### 4. 测试

MCP服务器目前支持一个基础工具：
- `ping` - 测试连接

## 下一步开发计划

- [ ] 实现WebSocket IPC通信
- [ ] 添加EditorPlugin和Dock UI面板
- [ ] 实现场景管理工具
- [ ] 实现脚本管理工具
- [ ] 添加Streamable HTTP传输支持

## 开发规范

- 遵循 `.repo_wiki/` 中的设计文档
- 遵循 `AGENTS.md` 中的架构说明
- 提交信息规范见 `user_rules`

## License

MIT
