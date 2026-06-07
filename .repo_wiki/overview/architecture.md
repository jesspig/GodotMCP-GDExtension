# 架构总览

项目是 C++ GDExtension 单进程架构，通过 MCP Streamable HTTP 直接暴露给 AI 客户端。

## 单进程设计

```mermaid
flowchart LR
    subgraph Client["AI 客户端<br/>(Claude Code / OpenCode / Cursor / Copilot / Codex / …)"]
        MCP[HTTP Client]
    end
    subgraph Godot["Godot Editor 进程"]
        DLL["godot_mcp_gdext.dll/.so/.dylib<br/>(extensions/src/, C++)"]
        subgraph Internal["内部组件"]
            HTTP["HttpServer<br/>(:9600, SSE)"]
            MCPHandler["McpHandler<br/>(JSON-RPC 2.0)"]
            Registry["HandlerRegistry<br/>(ITool 统一调度)"]
            Tools["built_in/tools/<br/>+ node_props/db/*.yaml<br/>+ node_resource/db/*.yaml"]
        end
        Main["主线程 process_frame<br/>→ HttpServer::poll()"]
    end
    MCP <-->|POST/GET/DELETE /mcp<br/>MCP Streamable HTTP| HTTP
    HTTP --> MCPHandler
    MCPHandler --> Registry
    Registry --> Tools
    Main --> HTTP
```

## 关键属性

| 维度 | 状态 |
|------|------|
| 进程数 | **1**（C++ GDExtension 加载到 Godot 编辑器内） |
| 传输 | MCP Streamable HTTP，端口 `:9600` |
| 工具注册 | `// @tool register` + `tools/codegen.py` 编译期自动注册 |
| 线程模型 | **纯主线程**（`EditorPlugin::_on_process_frame` 驱动） |
| 入口符号 | `gdext_rust_init`（`register_types.cpp:45`，遗留名） |
| 编码规范 | 根 `CMakeLists.txt:43` 已加 `/utf-8 /bigobj`（MSVC） |
| 构建优化 | sccache/ccache（自动检测）、PCH(MSVC)、Unity(jumbo)、lld-link |
| 持久化 | C++ 侧无独立状态；Godot 编辑器持有数据 |

## 数据流（一次工具调用）

```mermaid
sequenceDiagram
    participant AI as AI 客户端
    participant H as HttpServer
    participant M as McpHandler
    participant G as HandlerRegistry
    participant T as ITool
    AI->>H: POST /mcp (initialize)
    H->>M: handle_initialize()
    M-->>H: Session UUID + capabilities
    H-->>AI: 200 + MCP-Session-Id header
    AI->>H: GET /mcp (SSE stream)
    H-->>AI: 200 text/event-stream
    Note over AI,G: 工具调用
    AI->>H: POST /mcp (tools/call)
    H->>M: handle_message()
    M->>G: execute(name, args)
    G->>T: itool_table_ 查找 → execute(args)
    Note over T: 自动注入 ctx.root / ctx.node<br/>(按 needs_scene/needs_node)
    T-->>G: ToolResult 字典
    G-->>M: Dictionary result
    M-->>H: MCP content array
    H-->>AI: 200 application/json
```

## 当前目录布局

```
extensions/src/                  # C++ GDExtension 唯一源码根
├── register_types.cpp           # GDExtension 入口 (gdext_rust_init)
├── editor_plugin.cpp/.hpp       # McpEditorPlugin 生命周期 + process_frame 泵
├── pch.hpp                      # 预编译头 (STL + Godot 核心类型)
├── logging.hpp                  # 日志 inline 函数
├── built_in/
│   ├── tool_base.hpp/.cpp       # ITool + ToolResult + ToolContext
│   ├── cmd_utils.hpp/.cpp       # 共享工具（resolve_node / undoable_set / notify_file_changed）
│   ├── cmd_utils_json.cpp       # JSON↔Variant 递归转换
│   └── tools/                   # 所有 ITool 子类 (CMake GLOB 自动编译)
│       ├── meta/                #   5 个元工具（get_info / get_categories / get_tools / get_tool_detail / call_tool）
│       ├── node_tools/          #   资源工具模板
│       │   └── general/         #     6 个（load/clear/new/duplicate/save/get_resource_info）
│       ├── node_resource/       #   资源属性 YAML 数据库（419 文件）
│       │   └── db/
│       ├── group/               #   4 个分组工具
│       ├── signal/              #   4 个信号工具
│       ├── node_props/          #   节点属性 YAML 数据库（283 文件）+ 模板
│       │   ├── node_property_tool.hpp  # NodePropertyGetTool / NodePropertySetTool
│       │   └── db/                     # Node.yaml / CanvasItem.yaml / Label.yaml / ... (283 文件)
│       └── editor_tools/
│           └── scene_tree/      #   20 个场景树 CRUD 工具 + scene_tree_utils
├── server/
│   ├── ipc/
│   │   └── http_server.cpp/.hpp # MCP Streamable HTTP 服务器
│   ├── mcp/
│   │   └── mcp_handler.cpp/.hpp # JSON-RPC 2.0 会话管理
│   └── registry/
│       └── handler_registry.cpp/.hpp  # ITool 调度 + top_level_meta（3 个顶级分类）
├── sdk/
│   ├── mcp_tool_definition.hpp/.cpp   # GDScript/C# 可继承基类
│   └── mcp_tool_registry.hpp/.cpp     # 单例 SDK 注册表
├── lsp/
│   └── client.cpp/.hpp          # GDScript LSP 验证（StreamPeerTCP）
├── testing/
│   ├── test_engine.cpp/.hpp     # C++ 进程内测试引擎
│   ├── yaml_parser.hpp          # ryml → Godot Variant
│   ├── test_assertions.hpp      # 断言运行器
│   ├── godot_file_verifier.hpp  # 磁盘文件校验
│   └── type_utils.hpp           # 类型辅助
└── plugin/
    └── test_runner_dock.cpp/.hpp  # 编辑器底部面板（TestRunnerDock）

extensions/CMakeLists.txt        # FetchContent + codegen + add_library + 编译优化
tools/
├── codegen.py                   # // @tool register 扫描 + YAML 数据库 → 注册代码
└── collect_node_props.py        # Godot 运行时收集属性 → YAML 数据库

example/addons/godot_mcp/        # 构建产物（CMake 生成 + copy-gdext target）
├── plugin.cfg                   # 由根 CMakeLists.txt 从 PROJECT_VERSION 生成
├── godot_mcp.gdextension        # entry_symbol = gdext_rust_init
└── bin/                         # godot_mcp_gdext.{dll,so,dylib}（gitignored）
```

## 双重注册路径

```mermaid
flowchart LR
    subgraph BuiltIn["内置工具（C++ 侧）"]
        HPP["// @tool register 标记的 .hpp"]
        CODEGEN["tools/codegen.py<br/>(CMake 自定义命令)"]
        GEN["build/generated/<br/>generated_registration.cpp"]
        ITBL["HandlerRegistry::itool_table_<br/>(std::map)"]
        HPP --> CODEGEN --> GEN --> ITBL
    end
    subgraph SDK["SDK 自定义工具（GDScript / C#）"]
        DEF["继承 McpToolDefinition"]
        REG["McpToolRegistry::register_definition(this)<br/>或 register_tool(...)"]
        CUSTOM["McpToolRegistry 中带 custom_ 前缀的 CommandFn"]
        TBL["HandlerRegistry::table_<br/>(HashMap<String, CommandFn>)"]
        DEF --> REG --> CUSTOM --> TBL
    end
    subgraph Dispatch["统一调度"]
        EXEC["HandlerRegistry::execute(name, args)"]
        ITBL -.查 ITool.-> EXEC
        TBL -.未命中时查 CommandFn.-> EXEC
    end
```

详细命令路由与分类系统见 [modules/command-routing.md](../modules/command-routing.md)。
