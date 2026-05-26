# Cargo Workspace

## 结构

```toml
[workspace]
resolver = "2"
members = ["crates/core", "crates/gdext"]

[workspace.package]
version = "0.1.4"
edition = "2024"
```

**注意**：`crates/server` 已不存在——MCP 服务器现在是 Python/Cython 实现，位于 `server/` 目录。工作区仅包含两个 Rust crate。

工作区成员：

| Crate | 类型 | 说明 |
|-------|------|------|
| `crates/core` | lib | 共享协议类型 |
| `crates/gdext` | cdylib | Godot GDExtension |

## Python 服务器（非 Cargo 成员）

服务器代码在 `server/` 目录中：

```
server/
├── entry.pyx              # Cython --embed 入口 → .exe
├── pyproject.toml         # 用于 pytest 开发
├── setup.py               # 备用安装脚本
├── uv.lock                # 版本锁定（uv 依赖管理）
├── .venv/                 # 虚拟环境（含 Cython）
└── src/godot_mcp_server/
    ├── handler.py
    ├── bridge.py
    ├── registry.py
    ├── editor_ctl.py
    └── protocol.py
```

CMake 通过 Cython `--embed` 编译 Python 服务器为独立 `.exe`，不需要 Python 运行时（除 `python3xy.dll` 外）。

## 依赖

```toml
# crates/core/Cargo.toml
serde = "1"
serde_json = "1"
uuid = "1"

# crates/gdext/Cargo.toml
godot = "=0.5"              # 严格锁定
tokio = { version = "1", features = ["full"] }
tokio-tungstenite = "0.24"
futures-util = "0.3"
serde = "1"
serde_json = "1"
anyhow = "1"
regex = "1"
godot-mcp-core = { path = "../core" }
```

- **`godot = "=0.5"`**: 严格锁定——不兼容的更新会破坏构建
- `Cargo.lock` 已提交（用于二进制 crate）

## Rust 工具链

```toml
# rust-toolchain.toml
[toolchain]
channel = "stable"
components = ["rustfmt", "clippy"]
```

## Addon 清单

`example/addons/godot_mcp/` 目录是 Godot Editor 的 addon 目录：

```
example/addons/godot_mcp/
├── plugin.cfg              # 由 CMake 从 Cargo.toml 版本生成
├── godot_mcp.gdextension   # GDExtension 入口
└── bin/
    ├── godot_mcp_gdext.dll   # 构建产物（.gitignore 中）
    └── .gitkeep
```

`plugin.cfg`:

```ini
[plugin]
name="Godot MCP"
description="Model Context Protocol bridge for Godot Engine."
author=""
version="0.1.4"  # 从 Cargo.toml 自动同步
script=""        # 有意留空——全部逻辑是本机代码
```

`godot_mcp.gdextension`:

```ini
[configuration]
entry_symbol = "gdext_rust_init"
compatibility_minimum = "4.6"
reloadable = true

[libraries]
windows.debug.x86_64 = "res://addons/godot_mcp/bin/godot_mcp_gdext.dll"
windows.release.x86_64 = "res://addons/godot_mcp/bin/godot_mcp_gdext.dll"
linux.debug.x86_64 = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.so"
linux.release.x86_64 = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.so"
macos.debug = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib"
macos.release = "res://addons/godot_mcp/bin/libgodot_mcp_gdext.dylib"
```

## 构建与测试脚本

```bash
py -3 build.py                          # debug 构建 + addons.zip
py -3 build.py --release                # release 构建 + addons.zip
py -3 build.py --clean                  # cargo clean + 清空 addons/bin/
py -3 build.py --no-zip                 # 跳过打包（快速迭代）
py -3 build.py --no-server              # 仅重建 dll

cargo test --workspace                  # 离线测试（core + gdext，无需 Godot）
```

## Godot 测试项目

- 测试项目: `example/`（从仓库根访问）
- 编辑器图标: `example/icon.svg`
- 项目配置: `example/project.godot`