# 项目结构

## 目录布局

```
GodotMCP/
├── .repo_wiki/             # 项目知识库
├── extensions/
│   └── gdext/              # C++ GDExtension（活跃实现）
│       ├── src/
│       │   ├── commands/   # 工具处理器（16 个 handler 组）
│       │   ├── lsp/        # LSP 验证客户端
│       │   ├── ws/         # WebSocket 服务器
│       │   └── ...
│       └── CMakeLists.txt  # godot-cpp 10.0.0-rc1 FetchContent
├── crates/
│   ├── core/               # Rust 共享协议类型（遗留，仅用于测试）
│   └── gdext/              # Rust GDExtension（遗留，仅用于测试）
├── server/                 # Python MCP 服务器
│   ├── entry.py            # Cython --embed 入口 → .exe
│   ├── pyproject.toml      # 用于 pytest 开发
│   ├── .venv/              # 虚拟环境（含 Cython）
│   └── src/godot_mcp_server/
│       ├── handler.py
│       ├── bridge.py
│       ├── registry.py
│       ├── editor_ctl.py
│       └── protocol.py
├── example/                # Godot 测试项目
│   └── addons/godot_mcp/   # 构建输出目录
├── CMakeLists.txt          # 顶级 CMake 构建
├── Cargo.toml              # Rust workspace（遗留）
└── build.py                # 便捷构建脚本
```

## CMake 构建（主要）

顶级 `CMakeLists.txt`：
- `add_subdirectory(extensions/gdext)` → 构建 C++ GDExtension
- Cython `--embed` 编译 `server/entry.py` → `godot-mcp-server.exe`
- 生成 `plugin.cfg` + `godot_mcp.gdextension`
- 复制产物到 `example/addons/godot_mcp/bin/`
- CPack → `addons.zip`

## Cargo Workspace（遗留，仅用于测试）

```toml
[workspace]
resolver = "2"
members = ["crates/core", "crates/gdext"]
```

仅用于 `cargo test --workspace`。不会被部署。

## Addon 清单

`example/addons/godot_mcp/`：

```
example/addons/godot_mcp/
├── plugin.cfg              # 由 CMake 从 PROJECT_VERSION 生成
├── godot_mcp.gdextension   # GDExtension 入口（符号：gdext_rust_init）
└── bin/
    ├── godot_mcp_gdext.dll   # 构建产物（.gitignore 中）
    └── .gitkeep
```

## 构建与测试命令

```bash
py -3 build.py                          # debug 构建 + addons.zip
py -3 build.py --release                # release 构建 + addons.zip
py -3 build.py --clean                  # 清空 addons/bin/ + FetchContent 缓存
py -3 build.py --no-zip                 # 跳过打包（快速迭代）
py -3 build.py --no-server              # 仅重建 gdext dll

cargo test --workspace                  # Rust 遗留代码测试
cd server && pytest                     # Python 服务器测试
```

## Godot 测试项目

- 测试项目: `example/`（从仓库根访问）
- 编辑器图标: `example/icon.svg`
- 项目配置: `example/project.godot`
