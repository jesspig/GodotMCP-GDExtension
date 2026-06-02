# GodotMCP

MCP 服务器，通过 C++ GDExtension 将 Godot 4.6+ 编辑器暴露给 AI 工具。支持渐进式工具披露和 GDScript/C# 自定义工具扩展。

## 架构

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext.dll/.so/.dylib
                                            │
                                            ├── server/
                                            │   ├── HttpServer (TCPServer + HTTP/1.1)
                                            │   │   ├── /mcp → McpHandler (JSON-RPC 2.0 + SSE)
                                            │   │   └── /run-tests → TestEngine (YAML→执行→返回)
                                            │   ├── McpHandler (JSON-RPC 2.0 + SSE)
                                            │   └── HandlerRegistry (ITool + CommandFn)
                                            │
                                            ├── sdk/
                                            │   ├── McpToolDefinition (RefCounted, GDScript 可继承)
                                            │   ├── McpToolRegistry (单例, 工具注册中心)
                                            │   └── McpToolDefinitionAdapter (SDK→ITool 桥接)
                                            │
                                            ├── built_in/
                                            │   ├── tool_base.hpp (ITool 接口 + ToolResult + ToolContext)
                                            │   └── tools/ (124 个 ITool 子类, 每个 .hpp)
                                            │       ├── meta/            (5 个: godot_info + 渐进式披露)
                                            │       ├── scene/           (16 个场景 CRUD)
                                            │       ├── node/            (21 个节点操作)
                                            │       ├── property/        (21 个 2D 属性)
                                            │       ├── property_3d/     (6 个 3D 属性)
                                            │       ├── collision/       (2 个碰撞体)
                                            │       ├── find/            (4 个查找)
                                            │       ├── script_gd/       (5 个 GDScript)
                                            │       ├── script_cs/       (6 个 C#)
                                            │       ├── script_helpers/  (3 个脚本辅助)
                                            │       ├── project_settings/ (7 个设置)
                                            │       ├── project_settings_ext/ (10 个扩展设置)
                                            │       ├── editor_control/  (7 个编辑器控制)
                                            │       ├── input_map/       (4 个输入映射)
                                            │       ├── plugin_management/ (2 个插件管理)
                                            │       ├── undo/            (2 个撤销/重做)
                                            │       └── search/         (3 个搜索)
                                            │
                                            ├── testing/
                                            │   ├── TestEngine (非 MCP 工具, 独立类)
                                            │   ├── yaml_parser (ryml → Godot Variant)
                                            │   ├── test_assertions (断言引擎)
                                            │   └── godot_file_verifier (tscn/tres/project.godot)
                                            │
                                            ├── plugin/
                                            │   └── TestRunnerDock (C++ 底部面板, EditorPlugin)
                                            │
                                            └── McpEditorPlugin (EditorPlugin, 组装者)
```

## 关键设计

- **纯主线程**——无锁。`EditorPlugin::_on_process_frame()` 驱动 `HttpServer::poll()`。
- **ITool 接口**——所有内置工具继承 `ITool`，统一返回信封（`ToolResult::ok/err`）
- **注释驱动注册**——`// @tool register` 注释 + `codegen.py` 自动生成 `generated_registration.cpp`
- **统一调度**——`HandlerRegistry::execute(name, args)` 同时覆盖内置 ITool 和 SDK 自定义工具
- **两轴分类**——`source()`（meta/built_in/custom）决定可见性，`category()` 决定分组
- C++17，godot-cpp 10.0.0-rc1（FetchContent 固定），ryml（rapidyaml, FetchContent）
- 入口符号：`gdext_rust_init`（遗留名——不要修改 `.gdextension` 文件）

## 内置工具注册

工具在 `extensions/src/built_in/tools/<category>/<name>.hpp` 下，每个文件一个 `ITool` 子类：

```cpp
// @tool register
class MyTool : public ITool {
    String name() const override { return "my_tool"; }
    String category() const override { return "my_group"; }
    Dictionary execute_impl(const ToolContext &ctx) override { ... }
};
```

`codegen.py` 自动扫描 `// @tool register` 注释并生成注册代码。新增工具只需创建 `.hpp` 文件 + 加注释，**无需**改 `CMakeLists.txt`、`handler_registry.cpp`、`tool_schemas.json`。

目前已发现 **124 个内置工具**。

## 测试框架

### 整体架构

```
                        ┌──────────────────────┐
                        │  .test.yaml 配置文件   │
                        └──┬───────────────────┘
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
   ┌──────────────┐ ┌──────────┐ ┌────────────────┐
   │ C++ Test     │ │ 外部     │ │ 编辑器面板      │
   │ Engine       │ │ Python   │ │ TestRunnerDock  │
   │ (进程内)     │ │ 编排器   │ │ (C++ UI)        │
   └──────┬───────┘ └────┬─────┘ └───────┬────────┘
          └───────┬──────┘───────────────┘
                  ▼
         ┌──────────────────┐
         │  TestEngine::run │
         │  (YAML→执行→返回)│
         └──────────────────┘
```

### 入口

- **内部**：C++ `TestRunnerDock` 直接调用 `TestEngine::run(yaml)`
- **外部**：`POST /run-tests`（独立 HTTP 端点，不经过 MCP 协议）
- **三种入口共享同一套 `TestEngine` 实现、断言引擎、磁盘校验**

### TestEngine 执行流程

```
TestEngine::run(yaml_content)
  ├─ 1. ryml::parse → TestSuite
  ├─ 2. EditorFileSystem 快照 (纯内存遍历) → before_files
  ├─ 3. before_all 链
  ├─ 4. 测试循环:
  │     for each test:
  │       ├─ before_each 链
  │       ├─ result = HandlerRegistry::execute(tool, args)
  │       ├─ 追踪 result 中的路径 → tracked_paths
  │       ├─ 断言引擎校验 result vs expect
  │       ├─ 磁盘校验 (verify_scene_file / verify_project_godot / verify_raw_text)
  │       └─ after_each 链
  ├─ 5. after_all 链
  ├─ 6. EditorFileSystem 快照 → after_files
  ├─ 7. diff = after - before; to_delete = diff ∩ tracked_paths
  ├─ 8. 删除 to_delete + 空父目录清理
  └─ 9. 返回 { summary, results, cleanup }
```

### YAML 配置格式

```yaml
name: node_test
description: 节点创建与磁盘一致性验证

before_all:
  - tool: create_scene
    args: { name: "test", root_type: "Node2D" }

after_all:
  - tool: close_scene
    args: {}

tests:
  - tool: create_node
    description: 创建 Node2D
    args: { type: "Node2D", name: "test_node", parent: "." }
    expect:
      status: success
      has_keys: [node_path]
    disk_verify:
      scene:
        path: "res://scenes/test.tscn"
        nodes:
          - path: "test_node"
            type: "Node2D"
            exists: true
            properties:
              - path: "position"
                expect: [0, 0]
                type: Vector2
```

### 磁盘校验

| 文件格式 | Godot API | 校验方式 |
|----------|-----------|----------|
| `.tscn` | `ResourceLoader::load()` → `PackedScene` → `instantiate()` | 遍历节点树, `node.get(path)`, 类型转换后容差比较 |
| `.tres` | `ResourceLoader::load()` → `Resource` | 同上 |
| `project.godot` | `ConfigFile::load()` → `get_value()` | 逐 key 比较 |
| 任意文件 | `FileAccess::get_file_as_string()` | 子串包含/不包含 |

### 清理策略

- **双源追踪**：`EditorFileSystem` 快照差分（检测所有新增文件）+ 工具返回值路径追踪（区分测试/用户文件）
- **只删交集**：仅在两个来源中都出现的文件才删除
- **保护用户文件**：差分中但不在追踪中的文件跳过删除并在报告中 WARN

### 测试引擎文件结构

```
extensions/src/
├── testing/
│   ├── yaml_parser.hpp             # ryml → Godot Variant 递归转换
│   ├── test_assertions.hpp         # 断言引擎: status/has_keys/field_checks/error_contains
│   ├── godot_file_verifier.hpp     # 磁盘校验: 属性路径解析 + 类型转换 + 容差比较
│   └── test_engine.hpp/.cpp        # TestEngine: run/cleanup/snapshot/编排

├── server/ipc/
│   ├── http_server.hpp/.cpp        # 路由 /run-tests → TestEngine
│   └── test_http_handler.hpp       # HTTP YAML→JSON 编解码

├── plugin/
│   ├── test_runner_dock.hpp        # TestRunnerDock 声明 (VBoxContainer)
│   └── test_runner_dock.cpp        # 实现: 文件选择 + 运行 + Tree 结果
```

### 外部 Python 测试 (CI/开发)

```bash
# 启动 Godot 后:
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/scene.test.yaml

# 或用编排器 (管理 Godot 生命周期):
uv run python tests/test_orchestrator.py
```

旧 `tests/test_phases/` Python 阶段文件与新 YAML 测试在过渡期共存，最终被 YAML 替代。

## 构建

```bash
uv run python build.py                  # debug + addons.zip
uv run python build.py --release        # release + addons.zip
uv run python build.py --clean          # 清空 CMake 缓存（保留 _deps/）
uv run python build.py --clean-all      # 完全清除构建目录
uv run python build.py -j N             # 并行编译作业数
cmake --build build --target deep-clean # 仅清除 addons/bin/ + _deps/
```

- **Windows 关键**：必须用 `uv run python` 而非裸 `python`。
- Ninja 自动检测（Windows via vswhere + cl.exe），Unity Build（batch_size 8）已启用。
- 版本在根 `CMakeLists.txt:22` 设置。

## 端口

| 端口 | 用途 | 环境变量 |
|------|------|----------|
| 9600 | MCP Streamable HTTP + `/run-tests` | `GODOT_MCP_HTTP_PORT` |
| 6005 | Godot LSP（内置，仅用于验证） | — |

## C++ GDExtension 注意事项

- 所有处理器接收 `Dictionary args` 并返回 `Dictionary`。失败时返回含 `"error"` 键的 Dictionary。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()`。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `resolve_node()` 在 `cmd_utils.hpp` 中——接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- `variant_to_json()` / `json_to_variant()`——Godot Variant ↔ JSON 递归转换。
- `undoable_set()`——"立即应用 + 注册撤销" 惯用模式。

## 客户端配置

```json
{
  "mcpServers": {
    "godot": { "type": "streamable-http", "url": "http://localhost:9600/mcp" }
  }
}
```

## 文档

- `.repo_wiki/` — 项目知识库（架构、模块、测试、参考）
- `docs/` — Rspress 站点（中/英双语）
