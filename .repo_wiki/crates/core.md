# `crates/core` — 共享协议类型

> 被 `crates/gdext` 引用。不包含任何 Godot 或 MCP 运行时依赖。纯 serde JSON 类型。

```mermaid
classDiagram
    class IpcRequest {
        +String id
        +String method
        +Value params
    }
    class IpcResponse {
        +String id
        +IpcResult result
    }
    class IpcResult {
        <<enum>>
        Success { data: Value }
        Error { code: i32, message: String }
    }
    class IpcNotification {
        +String msg_type
        +String event
        +Value data
    }
    class ToolCallParams {
        +String tool
        +Value args
    }
    class ToolManifest {
        +Vec~ToolCategory~ categories
    }
    class ToolCategory {
        +String name
        +String display_name
        +Vec~ToolInfo~ tools
    }
    class ToolInfo {
        +String name
        +String description
        +Value input_schema
        +bool enabled
    }
    class ToolListUpdate {
        +Vec~ToolState~ tools
    }
    class ToolState {
        +String name
        +bool enabled
    }
    
    IpcResponse --> IpcResult : result
    ToolManifest --> ToolCategory : categories[]
    ToolCategory --> ToolInfo : tools[]
```

## 文件

### `protocol.rs`

| 类型 | 字段 | 说明 |
|------|------|------|
| `IpcRequest` | `id`, `method`, `params` | 从 server 发送到 gdext 的请求 |
| `IpcResponse` | `id`, `result: IpcResult` | 从 gdext 返回的响应（含 `status` tag） |
| `IpcResult` | `Success{data}` / `Error{code,message}` | 带标签的联合体 |
| `IpcNotification` | `msg_type`, `event`, `data` | 从 gdext 发送到 server 的通知 |
| `ToolCallParams` | `tool`, `args` | 工具调用参数封装 |

**注意**：旧的 `IpcRequest` 格式曾使用 `tool` + `args` + `id` 平面字段，现改为 `method` + `params`（`params` 包含 `tool` 和 `args`）。

### `tool_manifest.rs`

| 类型 | 说明 |
|------|------|
| `ToolManifest { categories }` | 分类组织的工具列表 |
| `ToolCategory { name, display_name, tools }` | 工具分类 |
| `ToolInfo { name, description, input_schema, enabled }` | 单个工具描述 |
| `ToolListUpdate { tools }` | 工具状态更新通知 |
| `ToolState { name, enabled }` | 单个工具启用状态 |

### 为什么需要 `core` crate

两侧通过 WebSocket 通信，使用 serde JSON 序列化/反序列化。共享类型确保两端格式严格一致。Python 侧 `protocol.py` 有对应的 Pydantic 模型与之匹配。

## 测试

`cargo test --workspace` 运行 14 个测试用例，覆盖所有类型的序列化/反序列化往返。