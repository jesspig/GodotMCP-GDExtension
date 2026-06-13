# Phase 2 — P2 差异化优势

> 预计工期：1-2 周。建立护城河。
> ADR-016 子决策：决策 8（安全模型增强）、决策 9（用户引导与分发体验）。

## P2-1：安全模型增强

### 背景

当前 `_confirm` 确认机制存在但内置工具均未标记 `is_destructive`（宏签名缺少此参数）。竞品 xulek/godotmcp 提供了 Guarded-action + 白名单机制。

### 修改清单

#### 1. 扩展 GODOT_MCP_TOOL 宏签名

```cpp
// register_itools.cpp — 修改前
#define GODOT_MCP_TOOL(cls, name_str, cat, is_meta_val, need_scene_val, need_node_val) \
    { auto tool = std::make_unique<cls>(); reg.register_tool(std::move(tool)); }

// register_itools.cpp — 修改后
#define GODOT_MCP_TOOL(cls, name_str, cat, is_meta_val, need_scene_val, need_node_val, is_destructive_val) \
    { auto tool = std::make_unique<cls>(); tool->set_is_destructive(is_destructive_val); reg.register_tool(std::move(tool)); }
```

> 注：`ITool` 基类需新增 `set_is_destructive(bool)` setter。

#### 2. 标记破坏性工具

在 `register_existing.hpp` 中，以下工具标记 `is_destructive = true`：

| 工具 | 原因 |
|------|------|
| `delete_node` | 删除节点不可逆 |
| `delete_file` | 删除文件不可逆 |
| `move_file` | 文件移动可能破坏引用 |
| `change_node_type` | 节点类型变更可能丢失配置 |
| `export_project` | 触发导出可能覆盖现有文件 |
| `set_game_node_property` | 运行时修改游戏状态 |

其余所有工具标记 `false`。

#### 3. 全局权限策略

新增环境变量 `GODOT_MCP_PERMISSION`：

| 值 | 行为 |
|---|------|
| `allow_all`（默认） | 所有工具直接执行，不拦截 |
| `confirm_destructive` | destructive 工具需 `_confirm=true` |
| `deny_destructive` | destructive 工具完全禁止 |

在 `mcp_handler.cpp` 的 `handle_tools_call()` 中检查此策略。

#### 4. 可选 Token 认证

新增环境变量 `GODOT_MCP_AUTH_TOKEN`：
- 未设置：不启用认证
- 已设置：所有请求必须带 `Authorization: Bearer <token>` 头

在 `http_server.cpp` 的请求分发前检查。

### 验证标准

1. `GODOT_MCP_PERMISSION=confirm_destructive` 时，`delete_node` 必须传 `_confirm=true`
2. `GODOT_MCP_PERMISSION=deny_destructive` 时，`delete_node` 返回 403
3. `GODOT_MCP_AUTH_TOKEN=xxx` 设置后，无 token 请求被 401 拒绝
4. 正确 token 的请求正常执行

---

## P2-2：客户端配置模板一键生成

### 背景

用户需要手动查阅文档来配置 `.mcp.json`。竞品 satelliteoflove/godot-mcp 提供了配置模板文档。

### 设计方案

新增 MCP 元工具 `generate_client_config`（`meta_tools` 分类）：

```json
{
    "name": "generate_client_config",
    "input_schema": {
        "type": "object",
        "properties": {
            "client": {
                "type": "string",
                "enum": ["claude_desktop", "claude_code", "cursor", "vscode_copilot", "windsurf", "cline", "opencode"],
                "description": "Target AI client"
            },
            "write_to_project": {
                "type": "boolean",
                "default": false,
                "description": "Auto-write config file to project root"
            }
        },
        "required": ["client"]
    }
}
```

**返回格式**：
```json
{
    "success": true,
    "data": {
        "client": "claude_code",
        "config_path": ".mcp.json",
        "config_content": "{\n  \"mcpServers\": {\n    \"godot\": {\n      \"url\": \"http://127.0.0.1:9600/mcp\"\n    }\n  }\n}",
        "instructions": "Add this to your project's .mcp.json file"
    }
}
```

**各客户端配置差异**：

| 客户端 | 传输方式 | 配置文件 | 格式 |
|--------|---------|---------|------|
| Claude Desktop | HTTP Streamable | `claude_desktop_config.json` | `{"mcpServers":{"godot":{"url":"http://127.0.0.1:9600/mcp"}}}` |
| Claude Code | HTTP Streamable | `.mcp.json` | 同上 |
| Cursor | HTTP Streamable | `.cursor/mcp.json` | 同上 |
| VS Code Copilot | HTTP Streamable | `.vscode/mcp.json` | 同上 |
| Windsurf | HTTP Streamable | `.windsurf/mcp.json` | 同上 |
| Cline | HTTP Streamable | `.clinerules/mcp.json` | 同上 |
| OpenCode | HTTP Streamable | `.opencode/opencode.json` | 同上 |

### 验证标准

1. `call_tool("generate_client_config", {"client": "claude_code"})` 返回正确的 `.mcp.json` 内容
2. `write_to_project=true` 时文件被写入项目根目录
3. 各客户端配置格式正确，AI 客户端可成功连接

---

## P2-3：请求限流

### 背景

无限流。恶意客户端可在 32 个连接内无限发送请求，导致编辑器主线程被 MCP 请求占满。

### 设计方案

简单令牌桶限流（per connection）：

```cpp
// http_connection.hpp
struct RateLimiter {
    int tokens = 30;                 // 当前令牌数
    double last_refill = 0.0;        // 上次填充时间
    static constexpr int kMaxTokens = 30;      // 最大令牌数
    static constexpr double kRefillRate = 30.0; // 每秒填充令牌数

    bool try_consume() {
        refill();
        if (tokens > 0) { tokens--; return true; }
        return false;
    }

    void refill() {
        double now = OS::get_singleton()->get_ticks_msec() / 1000.0;
        double elapsed = now - last_refill;
        tokens = Math::min(kMaxTokens, tokens + (int)(elapsed * kRefillRate));
        last_refill = now;
    }
};
```

在 `handle_request()` 前检查限流，超限返回 `429 Too Many Requests`。

### 验证标准

1. 单连接连续发送超过 30 个请求/秒时收到 429 响应
2. 正常使用频率（~5 请求/秒）不受影响


