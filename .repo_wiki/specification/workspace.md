# Cargo Workspace

## 结构

```toml
[workspace]
resolver = "2"
members = ["crates/core", "crates/server", "crates/gdext"]

[workspace.package]
version = "0.1.0"
edition = "2021"
```

工作区成员：

| Crate | 类型 | 说明 |
|-------|------|------|
| `crates/core` | lib | 共享协议类型 |
| `crates/server` | bin | MCP 服务器二进制 |
| `crates/gdext` | cdylib | Godot GDExtension |

## 锁定依赖

```toml
# crates/server/Cargo.toml
rmcp = "=1.7"          # 严格锁定
tokio = "1"
tokio-tungstenite = "0.18"
clap = { version = "4", features = ["derive"] }

# crates/gdext/Cargo.toml
godot = "=0.5"         # 严格锁定
tokio = "1"            # 与 server 匹配
tokio-tungstenite = "0.18"
```

- **`godot = "=0.5"`**: 严格锁定——不兼容的更新会破坏构建
- **`rmcp = "=1.7"`**: 严格锁定——重大 API 预期
- `Cargo.lock` 已提交（用于二进制 crate）

## Rust 工具链

```toml
# rust-toolchain.toml
[toolchain]
channel = "1.83.0"
```

CI 使用此版本来实现可复现的构建。

## Addon 清单

`godot/addons/godot_mcp/` 目录是 Godot Editor 的 addon 目录：

```
godot/addons/godot_mcp/
├── plugin.cfg          # 由 CMake 从 Cargo.toml 版本生成
├── godot_mcp.gdextension  # GDExtension 入口
└── bin/
    ├── godot_mcp_gdext.dll   # 构建产物（.gitignore 中）
    └── .gitkeep
```

`plugin.cfg`:

```ini
[plugin]
name="Godot MCP"
description="MCP bridge for Godot Editor"
author="..."
version="0.1.0"  # 从 Cargo.toml 自动同步
script=""        # 有意留空——全部逻辑是本机代码
```

`godot_mcp.gdextension`:

```ini
[configuration]
entry_symbol = "gdextension_init"

[libraries]
windows.debug.x86_64 = "res://addons/godot_mcp/bin/godot_mcp_gdext.dll"
windows.release.x86_64 = "res://addons/godot_mcp/bin/godot_mcp_gdext.dll"
```

## 目录引用

- 测试项目: `godot/`（从仓库根访问）
- Godot addon: `godot/addons/godot_mcp/`
- 测试场景: `godot/custom_nodes/` 等
- MCP 配置: 在各自客户端配置文件中

## 构建脚本

```bash
py -3 build.py              # 调试构建
py -3 build.py --release    # 发布构建
py -3 build.py --clean      # 清除构建产物
```
