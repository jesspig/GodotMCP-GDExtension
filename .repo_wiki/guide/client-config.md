# 客户端配置

## 相关页面

- [IPC 与 MCP 协议](../specification/protocol.md) — 协议兼容性
- [Dock UI 面板](../design/dock-ui.md) — 集成面板 UI

---

## 概述

Godot MCP Dock 面板中的 "Integration" 模块支持一键配置 12 个主流 MCP 客户端。每个客户端可独立选择 stdio 或 Streamable HTTP 传输协议，自动检测安装路径并写入对应格式的配置文件。

## 支持的 12 个客户端

| # | 客户端 | 默认传输 | 配置文件路径 | JSON Key |
|---|--------|----------|-------------|----------|
| 1 | Claude Code | stdio | `~/.claude/mcp.json` | `mcpServers` |
| 2 | Codex | HTTP | `~/.codex/config.toml` | `[mcp_servers."godot-mcp"]` (TOML) |
| 3 | Gemini CLI | HTTP | `~/.gemini/settings.json` 或 `.gemini/settings.json` | `mcpServers`, HTTP 用 `httpUrl` |
| 4 | OpenCode | stdio | `~/.config/opencode/config.json` 或项目级 `opencode.json` | `mcpServers` |
| 5 | Cursor | HTTP | `.cursor/mcp.json`（项目根） | `mcpServers` |
| 6 | GitHub Copilot | HTTP | `.vscode/mcp.json`（项目根） | **`servers`**（非 `mcpServers`）+ 必须 `type` |
| 7 | Qwen Code | HTTP | `~/.qwen/settings.json` 或 `.qwen/settings.json` | `mcpServers`, HTTP 用 `httpUrl` |
| 8 | Trae | HTTP | `.trae/mcp.json`（项目根） | `mcpServers` |
| 9 | Trae CN | HTTP | `.trae/mcp.json`（项目根，同 Trae） | `mcpServers` |
| 10 | Qoder | HTTP | Win: `%APPDATA%/Qoder/mcp-settings.json` / macOS: `~/Library/Application Support/Qoder/` / Linux: `~/.config/qoder/` | `mcpServers` |
| 11 | Antigravity | HTTP | `~/.gemini/antigravity/mcp_config.json` | `mcpServers`, HTTP 用 **`serverUrl`** |
| 12 | CodeBuddy | HTTP | `~/.codebuddy/.mcp.json` 或 `<project>/.mcp.json` | `mcpServers`, 需 `type` 字段 |

### 关键差异速查

| 客户端 | 特殊行为 |
|--------|---------|
| **GitHub Copilot** | JSON key 是 `servers` 而非 `mcpServers`；每个条目必须声明 `type: "stdio"` 或 `type: "http"` |
| **Antigravity** | HTTP URL 字段名是 `serverUrl` 而非 `url` |
| **Gemini CLI / Qwen Code** | HTTP URL 字段名是 `httpUrl`；`url` 用于旧版 SSE |
| **Codex** | 配置格式是 TOML 而非 JSON |
| **Trae CN** | 与 Trae 使用相同的配置路径和格式（`.trae/mcp.json`） |
| **Qoder** | 配置路径跨平台不同 |
| **CodeBuddy** | 需要显式 `type` 字段（`"stdio"` / `"sse"` / `"http"`） |

## 配置模板

### stdio 模式 — 通用（Claude Code / OpenCode）

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "/path/to/godot-mcp-server",
      "args": ["--transport", "stdio"],
      "env": {
        "GODOT_MCP_PORT": "9500"
      }
    }
  }
}
```

### stdio 模式 — GitHub Copilot（`servers` key + `type`）

```json
{
  "servers": {
    "godot-mcp": {
      "type": "stdio",
      "command": "/path/to/godot-mcp-server",
      "args": ["--transport", "stdio"],
      "env": {
        "GODOT_MCP_PORT": "9500"
      }
    }
  }
}
```

### stdio 模式 — CodeBuddy（需 `type`）

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "stdio",
      "command": "/path/to/godot-mcp-server",
      "args": ["--transport", "stdio"],
      "env": {
        "GODOT_MCP_PORT": "9500"
      }
    }
  }
}
```

### HTTP 模式 — 标准（Cursor / Trae / Trae CN / Qoder / CodeBuddy）

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP 模式 — Gemini CLI / Qwen Code（`httpUrl`）

```json
{
  "mcpServers": {
    "godot-mcp": {
      "httpUrl": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP 模式 — Antigravity（`serverUrl`）

```json
{
  "mcpServers": {
    "godot-mcp": {
      "serverUrl": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP 模式 — GitHub Copilot（`servers` key + `type: "http"`）

```json
{
  "servers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP 模式 — CodeBuddy（需 `type`）

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### Codex — TOML 格式（stdio）

```toml
[mcp_servers."godot-mcp"]
type = "stdio"
command = "/path/to/godot-mcp-server"
args = ["--transport", "stdio"]
env = { GODOT_MCP_PORT = "9500" }
enabled = true
```

### Codex — TOML 格式（HTTP）

```toml
[mcp_servers."godot-mcp"]
url = "http://127.0.0.1:8900/mcp"
enabled = true
```

## 客户端定义数据结构

```rust
// crates/gdext/src/dock/integration.rs
use godot::prelude::*;
use std::path::PathBuf;
use serde_json::Value;

#[derive(Clone, Copy)]
pub enum TransportType { Stdio, Http }

struct ClientDef {
    id: &'static str,
    name: &'static str,
    default_transport: TransportType,
    config_path_fn: fn(Option<&PathBuf>) -> Option<PathBuf>,
    write_stdio_fn: fn(&str, u16) -> Value,
    write_http_fn: fn(u16) -> Value,
}

const CLIENT_DEFINITIONS: &[ClientDef] = &[
    ClientDef {
        id: "claude_code",
        name: "Claude Code",
        default_transport: TransportType::Stdio,
        config_path_fn: |_| dirs::home_dir().map(|h| h.join(".claude").join("mcp.json")),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_url,
    },
    ClientDef {
        id: "codex",
        name: "Codex",
        default_transport: TransportType::Http,
        config_path_fn: |_| dirs::home_dir().map(|h| h.join(".codex").join("config.toml")),
        write_stdio_fn: write_codex_stdio_toml,
        write_http_fn: write_codex_http_toml,
    },
    ClientDef {
        id: "gemini_cli",
        name: "Gemini CLI",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj
            .map(|p| p.join(".gemini").join("settings.json"))
            .or_else(|| dirs::home_dir().map(|h| h.join(".gemini").join("settings.json"))),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_http_url,
    },
    ClientDef {
        id: "opencode",
        name: "OpenCode",
        default_transport: TransportType::Stdio,
        config_path_fn: |proj| proj
            .map(|p| p.join("opencode.json"))
            .or_else(|| dirs::config_dir().map(|c| c.join("opencode").join("config.json"))),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_url,
    },
    ClientDef {
        id: "cursor",
        name: "Cursor",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj.map(|p| p.join(".cursor").join("mcp.json")),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_url,
    },
    ClientDef {
        id: "github_copilot",
        name: "GitHub Copilot",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj.map(|p| p.join(".vscode").join("mcp.json")),
        write_stdio_fn: write_copilot_stdio,
        write_http_fn: write_copilot_http,
    },
    ClientDef {
        id: "qwen_code",
        name: "Qwen Code",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj
            .map(|p| p.join(".qwen").join("settings.json"))
            .or_else(|| dirs::home_dir().map(|h| h.join(".qwen").join("settings.json"))),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_http_url,
    },
    ClientDef {
        id: "trae",
        name: "Trae",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj.map(|p| p.join(".trae").join("mcp.json")),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_url,
    },
    ClientDef {
        id: "trae_cn",
        name: "Trae CN",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj.map(|p| p.join(".trae").join("mcp.json")),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_url,
    },
    ClientDef {
        id: "qoder",
        name: "Qoder",
        default_transport: TransportType::Http,
        config_path_fn: |_| {
            if cfg!(windows) {
                dirs::data_dir().map(|d| d.join("Qoder").join("mcp-settings.json"))
            } else if cfg!(target_os = "macos") {
                dirs::home_dir().map(|h| h.join("Library")
                    .join("Application Support")
                    .join("Qoder")
                    .join("mcp-settings.json"))
            } else {
                dirs::config_dir().map(|c| c.join("qoder").join("mcp-settings.json"))
            }
        },
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_url,
    },
    ClientDef {
        id: "antigravity",
        name: "Antigravity",
        default_transport: TransportType::Http,
        config_path_fn: |_| dirs::home_dir()
            .map(|h| h.join(".gemini").join("antigravity").join("mcp_config.json")),
        write_stdio_fn: write_stdio_mcp_servers,
        write_http_fn: write_http_server_url,
    },
    ClientDef {
        id: "codebuddy",
        name: "CodeBuddy",
        default_transport: TransportType::Http,
        config_path_fn: |proj| proj
            .map(|p| p.join(".mcp.json"))
            .or_else(|| dirs::home_dir().map(|h| h.join(".codebuddy").join(".mcp.json"))),
        write_stdio_fn: write_codebuddy_stdio,
        write_http_fn: write_codebuddy_http,
    },
];
```

### 写入函数分组

```rust
// 通用 mcpServers + command/args 格式
fn write_stdio_mcp_servers(server_path: &str, port: u16) -> Value { /* ... */ }

// 标准 HTTP url 格式 (Cursor, Trae, Trae CN, Qoder)
fn write_http_url(port: u16) -> Value {
    json!({"mcpServers": {"godot-mcp": {"url": format!("http://127.0.0.1:{}/mcp", port)}}})
}

// httpUrl 格式 (Gemini CLI, Qwen Code)
fn write_http_http_url(port: u16) -> Value {
    json!({"mcpServers": {"godot-mcp": {"httpUrl": format!("http://127.0.0.1:{}/mcp", port)}}})
}

// serverUrl 格式 (Antigravity)
fn write_http_server_url(port: u16) -> Value {
    json!({"mcpServers": {"godot-mcp": {"serverUrl": format!("http://127.0.0.1:{}/mcp", port)}}})
}

// GitHub Copilot: servers key + type field
fn write_copilot_stdio(server_path: &str, port: u16) -> Value { /* servers + type: "stdio" */ }
fn write_copilot_http(port: u16) -> Value {
    json!({"servers": {"godot-mcp": {"type": "http", "url": format!("http://127.0.0.1:{}/mcp", port)}}})
}

// CodeBuddy: mcpServers + type field
fn write_codebuddy_stdio(server_path: &str, port: u16) -> Value { /* mcpServers + type: "stdio" */ }
fn write_codebuddy_http(port: u16) -> Value {
    json!({"mcpServers": {"godot-mcp": {"type": "http", "url": format!("http://127.0.0.1:{}/mcp", port)}}})
}

// Codex: TOML 格式
fn write_codex_stdio_toml(server_path: &str, port: u16) -> Value { /* TOML string */ }
fn write_codex_http_toml(port: u16) -> Value { /* TOML string */ }
```

## 配置写入流程

```rust
pub fn configure_client(client_id: &str, transport: TransportType, http_port: u16) {
    let def = CLIENT_DEFINITIONS.iter().find(|c| c.id == client_id).unwrap();
    let project_root = find_project_root();
    let config_path = (def.config_path_fn)(project_root.as_ref());

    if let Some(path) = config_path {
        let server_path = find_server_binary();
        let config = match transport {
            TransportType::Stdio => (def.write_stdio_fn)(&server_path, http_port),
            TransportType::Http => (def.write_http_fn)(http_port),
        };

        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent).ok();
        }

        // Codex 使用 TOML，其余使用 JSON 合并
        if client_id == "codex" {
            write_toml_config(&path, &config);
        } else {
            write_json_config(&path, &config);
        }

        emit_signal("client_configured", client_id);
    }
}
```

## "Configure All" 行为

点击 [Configure All] 时：

1. 遍历 12 个客户端定义
2. 读取每个客户端在 UI 中选择的协议（stdio/HTTP）
3. 检测配置文件路径是否可写入（跳过不可达路径）
4. 生成对应格式配置并写入
5. 显示操作摘要报告：

```
=== Configure All Report ===
✅ Claude Code — ~/.claude/mcp.json 已更新 (stdio)
✅ Codex — ~/.codex/config.toml 已更新 (HTTP)
✅ Cursor — .cursor/mcp.json 已更新 (HTTP)
✅ GitHub Copilot — .vscode/mcp.json 已更新 (HTTP)
⏭️ Gemini CLI — 未安装 (跳过)
✅ OpenCode — config.json 已更新 (stdio)
✅ Qwen Code — .qwen/settings.json 已更新 (HTTP)
⏭️ Trae — 未安装 (跳过)
⏭️ Trae CN — 未安装 (跳过)
✅ Qoder — mcp-settings.json 已更新 (HTTP)
⏭️ Antigravity — 未安装 (跳过)
⏭️ CodeBuddy — 未安装 (跳过)

6 configured, 6 skipped
```

## 项目根目录检测

```rust
fn find_project_root() -> Option<PathBuf> {
    let editor = EditorInterface::singleton();
    let project_path = editor.get_resource_filesystem().get_filesystem_path("res://");
    let path = project_path.as_ref()?.get_path();
    Some(PathBuf::from(path.to_string()))
}
```

## 导出配置

[Export All Configs] 按钮将所有 12 个客户端的配置导出到剪贴板或 JSON 文件，每个客户端包含 stdio 和 HTTP 两种模板：

```json
{
  "claude_code": {
    "stdio": { "mcpServers": { "godot-mcp": { "command": "...", "args": ["--transport", "stdio"] } } },
    "http": { "mcpServers": { "godot-mcp": { "url": "http://127.0.0.1:8900/mcp" } } }
  },
  "codex": {
    "stdio": "[mcp_servers.\"godot-mcp\"]\ntype = \"stdio\"\ncommand = \"...\"",
    "http": "[mcp_servers.\"godot-mcp\"]\nurl = \"http://127.0.0.1:8900/mcp\""
  },
  "github_copilot": {
    "stdio": { "servers": { "godot-mcp": { "type": "stdio", "command": "..." } } },
    "http": { "servers": { "godot-mcp": { "type": "http", "url": "http://127.0.0.1:8900/mcp" } } }
  },
  "antigravity": {
    "http": { "mcpServers": { "godot-mcp": { "serverUrl": "http://127.0.0.1:8900/mcp" } } }
  }
}
```
