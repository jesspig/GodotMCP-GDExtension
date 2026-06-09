# GodotMCP

Godot 4.6+ 编辑器 MCP 服务器（C++ GDExtension，纯主线程无锁，HTTP :9600）。

## Build

```bash
uv run python build.py                  # Debug + addons.zip
uv run python build.py --release        # Release + addons.zip
uv run python build.py --clean          # 清 CMake 缓存（保留 _deps/）
uv run python build.py --clean-all      # 清除 build/（含 _deps/，慎用）
uv run python build.py --purge-cache    # 仅清除 _deps/（强制重下载，保留 build 缓存）
uv run python build.py --no-zip         # 跳过 zip 快速迭代
uv run python build.py -j N             # 并行编译作业数
cmake --build build --target deep-clean # 仅清 addons/bin/ + _deps/
```

- **始终用 `uv run python`**（非 `py -3`）。`uv` 保证依赖一致且自动激活 .venv。
- **陈旧缓存自动重试**：`build.py:145-147` 检测 MSB4019/VCTargetsPath 等错误，自动 `--clean` 后重试。
- **FetchContent 缓存**：`godot-cpp 10.0.0-rc1` + `ryml v0.7.0` 在 `build/_deps/`。`--clean` 和 `--clean-all` 均保留 `_deps/`，仅 `--purge-cache` 清除。
- **SSL 自动降级**：`build.py:168-175` 检测 `CRYPT_E_REVOCATION_OFFLINE` 等 schannel 错误，自动以 `CMAKE_TLS_VERIFY=0` 重试。
- **DLL 文件锁**：`example/addons/godot_mcp/bin/godot_mcp_gdext.dll` 被 Godot 编辑器持有，重建失败时先关编辑器或禁用插件。
- **构建优化**：sccache/ccache 自动检测（CMakeLists.txt:29-35）；Unity build 默认开启（batch_size 自动匹配 CPU 核心数）；PCH 已移除（Unity 已覆盖）；lld-link 自动检测。

## 关键约束

- **版本号**只在根 `CMakeLists.txt:22`（`PROJECT_VERSION "0.2.0-dev6"`）。`plugin.cfg` 与 `.gdextension` 由 CMake 自动生成。
- **入口符号** `gdext_rust_init`（`register_types.cpp:45`）——遗留名，**不要改**。
- **`compatibility_minimum = "4.6"`** 与 `GODOTCPP_API_VERSION "4.6"`（`extensions/CMakeLists.txt:15`）必须同步。
- **Pinned deps**: `godot-cpp 10.0.0-rc1`、`ryml v0.7.0`。升级前必须测试。

## 架构

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext (C++ GDExtension)
  POST /mcp, JSON-RPC 2.0 + SSE       McpEditorPlugin::_on_process_frame() 驱动 HttpServer::poll()
                                       工具由 codegen 自动注册（@tool register + YAML 数据库）
```

- **源码根**：`extensions/src/`（不是仓库根的 `src/`）。`editor_plugin.cpp:48` 调 `register_itools()`，由 codegen 生成的 `build/generated/generated_registration.cpp` 注入。
- **端口**：默认 9600，env `GODOT_MCP_HTTP_PORT` 覆盖（含范围校验）。
- **端点**：`/mcp`（JSON-RPC 2.0 + SSE），`/run-tests`（YAML 测试）。
- **双重注册**：`GDREGISTER` 注册 SDK 类 + EditorPlugin；`HandlerRegistry` 管理 `ITool` 主表与 SDK `CommandFn` 旁路表。
- **当前注册工具数**：~11734（84 个 @tool register + 283 节点属性 ×2 + 419 资源属性 ×2 + 844 设置项 ×2）。

## 添加内置工具

**CMake 自动编译**所有 `extensions/src/built_in/tools/**/*.cpp`。**codegen 自动注册**扫描 `// @tool register` 注释 + YAML 数据库。

```cpp
// extensions/src/built_in/tools/<category>/my_tool.hpp
// @tool register
class MyTool : public ITool {
    String name() const override { return "my_tool"; }
    String category() const override { return "node_tools"; }
    String brief() const override { ... }
    String description() const override { ... }  // 纯虚函数，必须实现
    Dictionary input_schema() const override { ... }
    bool is_meta() const override { return false; }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary execute_impl(const ToolContext &ctx) override { ... }
};
```

- **`description()` 是纯虚函数**，所有 `ITool` 子类必须实现。简单场景返回 `brief()`。
- **`// @tool register` 注释**是 codegen 识别的唯一标记，缺少则工具不注册。
- **顶级分类**硬编码于 `handler_registry.cpp` 的 `top_level_meta()`：`meta_tools`、`node_tools`、`editor_tools`。新顶级分类**必须**同步加 meta。
- **现有分类目录**：`meta/`、`group/`、`signal/`、`node_tools/general/`、`node_props/`、`node_resource/`、`editor_tools/scene_tree/`、`editor_tools/workspace/`、`editor_tools/filesystem/`、`editor_tools/settings/`。
- **场景树修改**：必须使用 `EditorUndoRedoManager`（通过 `EditorInterface::get_singleton()->get_editor_undo_redo()`），不用裸 `UndoRedo`。剪贴板通过 `PackedScene::pack()` / `instantiate()`。
- **写入文件**：不能直接写 `.tscn`，必须经 EditorInterface API 或写后调用 `notify_file_changed()`。

## YAML 数据库驱动的自动生成工具

三类工具通过 codegen 从 YAML 数据库自动生成注册代码，无需手动编写 `.hpp`：

| 类型 | YAML 目录 | 文件数 | 模板 |
|---|---|---|---|
| 节点属性 | `node_props/db/*.yaml` | 283 | `node_property_tool.hpp` → `NodePropertyGetTool`/`NodePropertySetTool` |
| 资源属性 | `node_resource/db/*.yaml` | 419 | `node_resource_tool.hpp` → `NodeResourceGetTool`/`NodeResourceSetTool` |
| 项目设置 | `editor_tools/settings/db/*.yaml` | 24 | `settings_tool.hpp` → `SettingGetTool`/`SettingSetTool` |

重新生成 YAML 数据库：
```bash
uv run python tools/collect_node_props.py --godot /path/to/godot
uv run python tools/collect_settings.py --godot /path/to/godot
```

## C++ 注意事项

- **MSVC UTF-8**：根 CMakeLists.txt:43 已加 `/utf-8 /bigobj`。非 ASCII 字符串字面量**必须**用 `String::utf8("中文")`，否则 MSVC 按 GBK 解释。
- **编辑器内部类**（`EditorDebuggerNode`、`EditorLog` 等）不在 godot-cpp 绑定中。用 `find_children("*", "ClassName", true, false)` 遍历场景树 + `call()` 动态调用。
- **常用 helper**（`cmd_utils.hpp`）：
  - `resolve_node()`：接受 `""`/`"."`/`"/"`/`"/root"`/根节点名/`"Root/Child"`
  - `undoable_set()`：修改节点属性优先用（立即应用 + 注册撤销）
  - `args_string/int/float/bool`：从 Dictionary 安全取参数
  - `variant_to_json` / `json_to_variant`：Variant ↔ JSON 递归转换
- **底部面板**：`add_control_to_bottom_panel` 在 godot-cpp 10.0.0-rc1 未绑定，用 `call()` 兜底（`editor_plugin.cpp:74-79`）。

## 测试

YAML 驱动，C++ `TestEngine` 在编辑器进程内执行，Python 编排器管理 Godot 生命周期。

```bash
cp tests/.env.example tests/.env        # 编辑 GODOT_PATH
pip install -r tests/requirements.txt
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/<name>.yaml  # 单文件测试
uv run python tests/test_orchestrator.py       # 完整套件
```

- **测试文件**：`tests/yaml_tests/*.yaml`（18 文件）。
- **清理**：双源追踪（EditorFileSystem 快照 + 工具返回值取交集），只删测试自建文件。
- **报告**：`tests/output/`（gitignored），JSON + Markdown。
- **带内 UI**：编辑器底部 "Tests" 面板可手工运行 YAML。
- **测试依赖**：`pytest`、`pytest-asyncio`、`httpx`、`python-dotenv`、`PyYAML`、`mcp`。

## Codegen

```bash
uv run python tools/codegen.py \
  --source-dir extensions/src/built_in/tools \
  --node-props-db extensions/src/built_in/tools/node_props/db \
  --resource-props-db extensions/src/built_in/tools/node_resource/db \
  --settings-db extensions/src/built_in/tools/editor_tools/settings/db \
  --output build/generated/generated_registration.cpp
```

- 扫描 `.hpp` 中的 `// @tool register`，正则 `^class\s+(\w+)\s*[:]` 提取类名。
- **UTF-8 BOM 会导致 codegen 无法识别 `// @tool register`**。用 `Set-Content`（PowerShell）创建的文件带 BOM，必须用 `$PSDefaultParameterValues['Out-File:Encoding']='utf8'` 或 Python 写入。
- CMake 自动在 `extensions/CMakeLists.txt` 中驱动 codegen 作为自定义命令。

## 文档

- `docs/` — Rspress 站点（中/英双语，`i18n.json` 驱动）。`pnpm run dev` / `pnpm run build`。
- `.repo_wiki/` — 项目知识库（架构、ADR、变更日志）。优先查这里获取设计细节。
- [项目 Wiki](.repo_wiki/index.md) — 模块文档索引、快照数据、Agent 上手指南。

## 本地开发 MCP 连接

`.opencode/opencode.json` 已预配本地 Godot MCP 连接：
```json
{"mcp": {"godot-mcp": {"type": "remote", "url": "http://127.0.0.1:9600/mcp"}}}
```
启动 Godot 编辑器并启用插件后，即可通过 Godot MCP 工具直接操作编辑器。

## 市场分析与优化路线图

### 竞品生态全景

市面 20+ Godot MCP 项目按技术路线分四大阵营：

| 阵营 | 代表项目 | Stars | 工具数 | 特点 |
|------|---------|-------|--------|------|
| **Node.js + GDScript Plugin** | Coding-Solo/godot-mcp | **4,068** | ~20 | 最热门，npx 一键启动 |
| | alexmeckes/godot-mcp | ~150 | **99** | Shader/Animation/Nav/UI/Procedural 生成 |
| | Sods2/godot-mcp | ~120 | **77** | **断点调试/堆栈跟踪/单步执行** |
| | satelliteoflove/godot-mcp | ~80 | 11 | 观察优先，截图/运行时/动画/TileMap |
| | tomyud1/godot-mcp | ~200 | **42** | **交互式项目可视化器** |
| | Dreamer568/godot-mcp | ~60 | ~80 | 资产分配强，每秒写 godot_state.json |
| | Djentinga/godot-mcp | ~15 | **149** | Coding-Solo 极限 fork |
| **Python + GDScript Plugin** | xulek/godotmcp | ~20 | ~70 | 守卫操作、工作流检查点 |
| | Rufaty/godot-mcp-enhanced | ~17 | ~40 | 输入模拟、插件检测、性能监控 |
| **C++ GDExtension（同阵营）** | **本项目 GodotMCP** | — | **~11,734** | **C++ 进程内、纯主线程、Codegen** |
| | MeowMeowZi/meow-godot-mcp | 2 | **50** | 架构最接近，有游戏桥接/输入/截图/TileMap/UI/动画 |
| | nklisch/theatre | 0 | 47 | Rust 实现，spatial snapshot 概念 |
| **其他** | Sharks820/godot-mcp-ultimate | ~10 | 47+15 子 Agent | 死代码检测/信号流分析/项目健康面板 |

### 功能缺口（vs 竞品）

**P0 — 必须补（竞品都有，缺失即硬伤）：**

| 能力 | 本项目 | Meow | alexmeckes | Sods2 | satellite |
|------|--------|------|------------|-------|-----------|
| 游戏运行时桥接 | ❌ | ✅ | ✅ | ✅ | ✅ |
| 编辑器/游戏截图 | ❌ | ✅ | ✅ | ✅ | ✅ |
| 脚本读写编辑 | ❌ | ✅ | ✅ | ✅ | ❌ |

**P1 — 应该补（显著提升竞争力）：**

| 能力 | 本项目 | Meow | alexmeckes | Sods2 | satellite |
|------|--------|------|------------|-------|-----------|
| 动画系统工具 | ❌ | ✅ | ✅ | ❌ | ✅ |
| UI/Control 工具 | ❌ | ✅ | ✅ | ❌ | ❌ |
| TileMap 操作 | ❌ | ✅ | ❌ | ❌ | ✅ |
| 碰撞形状一键创建 | ❌ | ✅ | ❌ | ❌ | ❌ |

**P2 — 加分项（差异化竞争）：**

| 能力 | 本项目 | 拥有者 |
|------|--------|--------|
| 断点调试集成 | ❌ | Sods2（独有） |
| MCP Resources + Prompts | ❌ | Meow、ee0pdt |
| 项目可视化器 | ❌ | tomyud1（独有） |
| Godot 文档查询 | ❌ | satelliteoflove、dreamvision-dev |
| 项目脚手架 | ❌ | MhrnMhrn |
| 导出/插件管理 | ❌ | Raunaksplanet |
| 输入映射管理 | ❌ | Funplay |
| Shader 工具 | ❌ | alexmeckes（11 种预设） |

### 核心竞争力

本项目**唯一不可替代的优势**：**C++ GDExtension 进程内运行 + 零外部依赖**。

- 启动速度：零（插件随编辑器加载）
- 调用延迟：函数调用级别，非 IPC
- 资源开销：无额外进程
- 部署复杂度：一个 `.dll` 文件

### 优化路线图

```
Phase 1 (P0) — 入场券
├── 游戏运行时桥接（EditorDebuggerPlugin + TCP/WS）
│   ├── 输入注入（键盘/鼠标/Action）
│   ├── 运行时属性读取
│   ├── 运行时场景树获取
│   └── GDScript 表达式执行
├── 编辑器/游戏截图（EditorInterface viewport → ImageContent）
└── 脚本读写编辑（read/write/edit/patch/attach/detach）

Phase 2 (P1) — 竞争力
├── 动画系统（AnimationPlayer + AnimationLibrary + 轨道/关键帧 CRUD）
├── UI/Control 工具（Control 创建、布局预设、主题覆盖、StyleBox）
├── TileMap 操作（批量放置/擦除/查询）
└── 碰撞形状一键创建（CollisionShape2D/3D + 9 种形状）

Phase 3 (P2) — 差异化
├── 断点调试集成（set_breakpoint / stack_trace / step_over）
├── MCP Resources + Prompts（godot:// 资源 + 工作流模板）
├── 项目可视化器（力导向图 + 场景图浏览器）
├── Godot 文档查询（get_class_info / search_docs）
├── 项目脚手架（create_project）
├── 导出/插件/输入映射管理
└── Shader 工具（预设效果模板）
```

详细规划见 `.repo_wiki/design/roadmap.md`（含追踪清单）。
