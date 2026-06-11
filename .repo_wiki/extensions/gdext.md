# `extensions/src` — GDExtension C++ 实现（当前活跃）

> 加载到 Godot 编辑器内的本机插件。使用 `godot-cpp 10.0.0-rc1` 构建。**这是项目唯一的 GDExtension 实现**。

## 组件图

```mermaid
flowchart TB
    subgraph Godot["Godot Editor Process"]
        subgraph DLL["godot_mcp_gdext.dll / .so / .dylib"]
            EP["editor_plugin.cpp<br/>McpEditorPlugin"]
            HTTP["server/ipc/http_server.cpp<br/>HttpServer (:9600)"]
            MCP["server/mcp/mcp_handler.cpp<br/>McpHandler"]
            REG["server/registry/handler_registry.cpp<br/>HandlerRegistry"]
            BASE["built_in/tool_base.hpp<br/>ITool + ToolResult + ToolContext"]
            TOOLS["built_in/tools/<br/>~149 个 .hpp (X-macro 注册)"]
            UTILS["built_in/cmd_utils.hpp<br/>resolve_node / undoable_set / notify_file_changed"]
            SDK["sdk/<br/>McpToolDefinition / McpToolRegistry"]
            LSP["lsp/client.cpp<br/>GDScript 验证"]
            TEST["testing/test_engine.cpp<br/>YAML 进程内引擎"]
            BRIDGE["runtime/bridge.cpp<br/>RuntimeBridge (TCP :9601)"]
        end
    end

    EP --> HTTP
    HTTP --> MCP
    MCP --> REG
    REG --> TOOLS
    REG --> SDK
    TOOLS --> UTILS
    TOOLS --> BASE
    EP -.->|_process() 每帧驱动| HTTP
    EP -.->|_process() 每帧驱动| BRIDGE
    SDK -->|register_custom_tool → IToolAdapter| REG
    TOOLS -->|Godot API| EP

    subgraph Game["Godot Game Process"]
        GB["runtime/game_bridge.cpp<br/>GameBridgeNode (TCP :9601)"]
    end
    BRIDGE <-->|TCP JSON| GB
```

## 文件结构

```
extensions/src/
├── register_types.cpp              # GDExtension 入口：gdext_mcp_init
├── editor_plugin.cpp/.hpp          # McpEditorPlugin 生命周期 + _process() 泵
├── pch.hpp                         # 预编译头（STL + Godot 核心类型）
├── logging.hpp                     # log_info/warn/error（28 行）
├── built_in/
│   ├── register_itools.cpp         # X-macro 注册主文件（#include + GODOT_MCP_TOOL 宏）
│   ├── tool_base.hpp/.cpp          # ITool + ToolResult + ToolContext
│   ├── tool_adapter.hpp/.cpp       # IToolAdapter（SDK Callable → ITool 适配器）
│   ├── cmd_utils.hpp/.cpp          # 共享工具（resolve_node、undoable_set、notify_file_changed）
│   ├── cmd_utils_json.cpp          # JSON↔Variant 递归转换
│   ├── screenshot_utils.hpp        # 截图捕获
│   └── tools/                      # 所有 ITool 子类（CMake GLOB 自动编译 .cpp）
│       ├── register/               # X-macro 注册文件（4 个）
│       │   ├── register_meta.hpp
│       │   ├── register_existing.hpp
│       │   ├── register_fallback.hpp
│       │   └── register_docs.hpp
│       ├── meta/                   #   6 个元工具
│       ├── signal/                 #   4 个信号工具
│       ├── group/                  #   4 个分组工具
│       ├── node_tools/general/     #   6 个资源管理工具
│       ├── node_tools/             #   node_resource_tool（模板）
│       ├── node_props/             #   node_property_tool（模板）+ YAML 数据库
│       ├── node_properties/        #   2 个通用兜底工具（Layer 0）
│       ├── editor_tools/
│       │   ├── scene_tree/         #   25+ 场景树 CRUD 工具 + utils
│       │   ├── animation/          #   5 个动画工具
│       │   ├── control/            #   4 个 UI/Control 工具
│       │   ├── collision/          #   1 个碰撞形状工具
│       │   ├── docs/               #   8 个文档查询工具（Layer 3）
│       │   ├── export/             #   2 个导出工具
│       │   ├── filesystem/         #   12 个文件系统工具
│       │   ├── inputmap/           #   1 个输入映射工具
│       │   ├── plugin/             #   3 个插件管理工具
│       │   ├── scaffold/           #   1 个脚手架工具
│       │   ├── scripts/            #   13 个脚本读写验证工具
│       │   ├── settings/           #   4 个设置工具
│       │   ├── shader/             #   3 个 shader 工具
│       │   ├── tilemap/            #   3 个 TileMap 工具
│       │   ├── visualizer/         #   1 个可视化工具
│       │   └── workspace/          #   29 个工作区/调试器工具
│       └── runtime_tools/
│           ├── bridge/             #   6 个运行时桥接工具
│           └── lifecycle/          #   6 个游戏生命周期工具
├── server/
│   ├── ipc/
│   │   ├── http_server.cpp/.hpp    # MCP Streamable HTTP 服务器（:9600）
│   │   ├── http_parser.cpp         # HTTP 请求行+头部解析
│   │   ├── http_connection.cpp     # TCP 连接管理 + 响应发送
│   │   ├── http_sse.cpp            # SSE 事件流
│   │   └── test_http_handler.hpp   # /run-tests 端点
│   ├── mcp/
│   │   └── mcp_handler.cpp/.hpp    # MCP JSON-RPC 2.0 会话管理
│   └── registry/
│       └── handler_registry.cpp/.hpp  # ITool 调度 + 分类自动发现 + 搜索引擎
├── runtime/
│   ├── bridge.hpp/.cpp             # RuntimeBridge：编辑器侧 TCP 客户端（→9601）
│   └── game_bridge.hpp/.cpp        # GameBridgeNode：游戏进程 TCP 服务端（:9601）
├── sdk/
│   ├── mcp_tool_definition.hpp/.cpp  # GDScript 可继承的 RefCounted 基类
│   └── mcp_tool_registry.hpp/.cpp    # 单例注册表
├── lsp/
│   └── client.cpp/.hpp             # GDScript LSP 验证（StreamPeerTCP）
└── testing/
    ├── test_engine.cpp/.hpp        # 进程内 YAML 测试引擎
    ├── yaml_parser.hpp             # ryml → Godot Variant 解析
    ├── test_assertions.hpp         # 断言引擎（status/has_keys/field_checks/error_contains）
    ├── godot_file_verifier.hpp     # .tscn/project.godot 磁盘验证
    └── type_utils.hpp              # 类型辅助
```

## 工具注册（X-macro 分文件）

`editor_plugin.cpp` 调用 `register_itools(registry_)`。注册机制是 X-macro：

1. `register_itools.cpp` 包含所有工具 `.hpp` 的 `#include` 指令
2. 定义 `GODOT_MCP_TOOL` 宏，展开为 `reg.register_tool(std::make_unique<cls>())`
3. 通过 `#include` 展开四个注册文件：
   - `register/register_meta.hpp` — 6 个元工具
   - `register/register_existing.hpp` — ~120 个功能工具
   - `register/register_fallback.hpp` — 2 个后备属性工具
   - `register/register_docs.hpp` — 8 个文档查询工具

**添加新内置工具的完整流程**：

1. 在 `extensions/src/built_in/tools/<category>/` 创建 `<name>.hpp`，实现 ITool 接口
2. 在 `extensions/src/built_in/tools/register/` 对应 X-macro 文件加一行：
   ```cpp
   GODOT_MCP_TOOL(MyTool, "my_tool", "editor_tools/my_category", false, false, false)
   ```
3. 在 `extensions/src/built_in/register_itools.cpp` 对应分类区域加 `#include`
4. 编译 —— CMake 自动 GLOB `tools/**/*.cpp`（如有）+ X-macro 编译时注册

**无需** codegen，**无需** `// @tool register` 注释。

## 顶级分类

`HandlerRegistry` 从工具的 `category()` 字符串自动发现顶级分类（`/` 分割取首段），通过 `prettify_segment()` 美化 label：

| category | label | description |
|----------|-------|-------------|
| `meta_tools` | Meta Tools | 元工具与系统信息查询 |
| `node_tools` | Node Tools | 节点/资源属性读取与修改工具 |
| `editor_tools` | Editor Tools | 编辑器操作工具：场景树、脚本、调试器等 |
| `runtime_tools` | Runtime Tools | 游戏运行时桥接与生命周期管理 |

## `cmd_utils.hpp` 共享工具函数

| 函数 | 说明 |
|------|------|
| `resolve_node(root, path)` | 节点路径解析：接受 `""`, `"."`, `"/"`, 根节点名, `"Root/Child"` |
| `get_root()` | 获取当前编辑场景根节点 |
| `get_root_or_error(out_error)` | 同上，失败时填 error |
| `get_undo_redo()` | 编辑器 UndoRedo 管理器 |
| `undoable_set(node, prop, val, action)` | "立即应用 + 注册撤销"惯用模式 |
| `mark_scene_dirty()` | 标记当前场景未保存 |
| `notify_file_changed(path)` | 通知 EditorFileSystem 文件变更 |
| `save_version_marker(root)` | 记录当前 undo 版本作为已保存标记 |
| `collect_owner_warnings(root)` | 收集无 owner 的节点警告 |
| `relative_path(root, node)` | 编辑器路径 → 场景相对路径 |
| `globalize_path(path)` | res:// → 绝对磁盘路径 |
| `ensure_parent_dir(path)` | 创建 res:// 路径的父目录 |
| `args_string/int/float/bool` | 类型安全的参数读取 |
| `variant_to_json(Variant)` | Variant → JSON 递归展开 |
| `json_to_variant(Variant)` | JSON → Godot Variant 还原 |
| `collect_nodes_by(node, pred, ...)` | 通用树遍历（lambda 谓词） |
| `walk_project_dir(dir, exts, ...)` | 通用目录遍历 |

## 构建关键（`extensions/CMakeLists.txt`）

| 行 | 内容 |
|:--:|------|
| L15 | `set(GODOTCPP_API_VERSION "4.6")` |
| L17-21 | `FetchContent` 拉取 `godot-cpp 10.0.0-rc1` |
| L30-42 | `FetchContent` 拉取 `rapidyaml v0.7.0`（GIT_SUBMODULES 包含 c4core） |
| L49-86 | `GODOT_MCP_SOURCES`（含 register_types / editor_plugin / server / sdk / lsp / testing） |
| L91 | `file(GLOB_RECURSE TOOL_SOURCES "src/built_in/tools/*.cpp")` — 自动收集 .cpp |
| L96 | `add_library(godot_mcp_gdext SHARED ...)` |
| L107-112 | 编译定义 `GODOT_MCP_PLUGIN_VERSION` |
| L117-146 | Unity Build ON（batch size 自动匹配 CPU 核数，上限 32） |
| L156-162 | lld-link 自动检测（MSVC） |
| L166 | MSVC: `/utf-8 /bigobj /W3 /wd4244 /wd4267` |
