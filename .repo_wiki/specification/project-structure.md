# 项目结构

## 目录布局

```
GodotMCP/
├── .repo_wiki/             # 项目知识库
├── extensions/
│   └── gdext/              # C++ GDExtension（唯一实现）
│       ├── src/
│       │   ├── commands/   # 工具处理器（17 个文件，16 组活跃注册）
│       │   ├── ipc/        # WsServer + HttpServer
│       │   ├── mcp/        # McpHandler（JSON-RPC 2.0 会话管理）
│       │   ├── lsp/        # LSP 验证客户端
│       │   ├── protocol/   # IPC 协议构造辅助
│       │   ├── register_types.cpp  # GDExtension 入口
│       │   ├── editor_plugin.cpp   # McpEditorPlugin 生命周期
│       │   └── logging.hpp         # 日志 inline 函数
│       └── CMakeLists.txt  # godot-cpp 10.0.0-rc1 FetchContent
├── server/                 # Python MCP 服务器
│   ├── entry.py            # Cython --embed 入口 → .exe
│   ├── tools/
│   │   └── patch_entry_c.py     # 将 PYTHONHOME 写入 entry.c
│   ├── src/godot_mcp_server/
│   │   ├── handler.py      # GodotMcpHandler 分发器
│   │   ├── bridge.py       # GodotBridge WebSocket 客户端
│   │   ├── registry.py     # ToolRegistry (125 tools)
│   │   ├── editor_ctl.py   # 编辑器进程管理
│   │   └── protocol.py     # Pydantic IPC 协议模型
│   └── tests/              # pytest 测试
├── tools/
│   └── tool_schemas.json   # 从 registry.py 生成的工具 schema（CMake 复制到 addon）
├── example/                # Godot 测试项目
│   └── addons/godot_mcp/   # 构建输出目录
│       ├── plugin.cfg              # 由 CMake 从 PROJECT_VERSION 生成
│       ├── godot_mcp.gdextension   # GDExtension 入口（符号：gdext_rust_init）
│       └── bin/                    # 构建产物（.gitignore 中）
├── docs/                   # 文档
├── CMakeLists.txt          # 顶级 CMake 构建（版本唯一来源）
├── build.py                # 便捷构建脚本（argparse 包装 CMake）
├── build/                  # CMake 构建输出
├── pyproject.toml          # Python 依赖与工具配置（在根目录，不是 server/）
├── README.md               # 英文 README
├── README-zh.md            # 中文 README
├── License                 # MIT 许可证
└── uv.lock                 # uv 锁定文件
```

## CMake 构建（唯一构建系统）

顶级 `CMakeLists.txt`：
- `add_subdirectory(extensions/gdext)` → 构建 C++ GDExtension
- Cython `--embed` 编译 `server/entry.py` → `godot-mcp-server.exe`
- 生成 `plugin.cfg` + `godot_mcp.gdextension`
- 复制产物到 `example/addons/godot_mcp/bin/`
- CPack → `addons.zip`

## 构建与测试命令

```bash
py -3 build.py                          # debug 构建 + addons.zip
py -3 build.py --release                # release 构建 + addons.zip
py -3 build.py --clean                  # 清空 CMake 缓存（保留 _deps/）
py -3 build.py --no-zip                 # 跳过打包（快速迭代）
py -3 build.py --no-server              # 仅重建 gdext dll

pytest                                   # Python 服务器测试（从根目录运行）
```

## Godot 测试项目

- 测试项目: `example/`（从仓库根访问）
- 编辑器图标: `example/icon.svg`
- 项目配置: `example/project.godot`
