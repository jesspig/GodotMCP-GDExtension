# 项目结构

## 目录布局

```
GodotMCP/
├── .opencode/              # OpenCode 配置
├── .repo_wiki/             # 项目知识库
├── extensions/
│   └── gdext/              # C++ GDExtension（唯一代码库）
│       ├── src/
│       │   ├── commands/   # 工具处理器（17 个文件，16 组活跃注册）
│       │   ├── ipc/        # HttpServer
│       │   ├── mcp/        # McpHandler（JSON-RPC 2.0 会话管理 + 错误码）
│       │   ├── lsp/        # LSP 验证客户端
│       │   ├── register_types.cpp  # GDExtension 入口
│       │   ├── editor_plugin.cpp   # McpEditorPlugin 生命周期
│       │   └── logging.hpp         # 日志 inline 函数
│       └── CMakeLists.txt  # godot-cpp 10.0.0-rc1 FetchContent
├── tools/
│   └── tool_schemas.json   # 工具 schema（CMake 复制到 addon）
├── example/                # Godot 测试项目
│   └── addons/godot_mcp/   # 构建输出目录
│       ├── plugin.cfg              # 由 CMake 从 PROJECT_VERSION 生成
│       ├── godot_mcp.gdextension   # GDExtension 入口（符号：gdext_rust_init）
│       └── bin/                    # 构建产物（.gitignore 中）
├── docs/                   # 文档
├── CMakeLists.txt          # 顶级 CMake 构建（版本唯一来源）
├── build.py                # 便捷构建脚本（argparse 包装 CMake）
├── build/                  # CMake 构建输出
├── pyproject.toml          # Python 构建工具依赖
├── README.md               # 英文 README
├── README-zh.md            # 中文 README
├── License                 # MIT 许可证
└── uv.lock                 # uv 锁定文件
```

## CMake 构建（唯一构建系统）

顶级 `CMakeLists.txt`：
- `add_subdirectory(extensions/gdext)` → 构建 C++ GDExtension
- 生成 `plugin.cfg` + `godot_mcp.gdextension`
- 复制产物到 `example/addons/godot_mcp/bin/`
- CPack → `addons.zip`

## 构建与测试命令

```bash
py -3 build.py                          # debug 构建 + addons.zip
py -3 build.py --release                # release 构建 + addons.zip
py -3 build.py --clean                  # 清空 CMake 缓存（保留 _deps/）
py -3 build.py --no-zip                 # 跳过打包（快速迭代）
```

## Godot 测试项目

- 测试项目: `example/`（从仓库根访问）
- 编辑器图标: `example/icon.svg`
- 项目配置: `example/project.godot`
