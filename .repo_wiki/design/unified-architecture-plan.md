# 统一工具架构重构计划

> 将 GodotMCP 的 119 个内置工具从自由函数 + JSON Schema 两阶段注册，重构为基于 `ITool` 接口、组合式能力声明、注释驱动自动注册的统一架构，实现内置工具与 SDK 自定义工具的完全同构。

## 背景与动机

### 现状痛点

| 问题 | 表现 |
|------|------|
| **返回结构不统一** | `make_success()`、`make_error()`、裸 `Dictionary{"x":1,"y":2}` 混用，客户端需要为每个工具定制解析逻辑 |
| **两阶段注册** | `register_tool(name, fn)` 注册函数 → `load_schemas_from_json()` 从 `tool_schemas.json` 加载元数据；两者不同步则工具不可见 |
| **样板代码重复** | 约 60 处 `get_root()` + `resolve_node()` + error check 拷贝粘贴在各 `cmd_*.cpp` 中 |
| **新增工具需改 3 处** | 新 `cmd_*.cpp` + `extensions/CMakeLists.txt` + `handler_registry.cpp` 前向声明 |
| **无输出 Schema** | 所有工具只有输入 Schema，输出格式全靠客户端猜测 |
| **测试用例代码重复** | 18 个 phase 文件各自实现 JSON 解析、参数传递、结果断言 |
| **内置与 SDK 工具分叉** | `HandlerRegistry` 中 `table_`（CommandFn）和 `tool_info_`（ToolInfo）分离存储，SDK 工具走 `register_custom_tool()` 独立路径 |

### 设计目标

1. **统一接口**：所有工具（内置/SDK）实现同一 `ITool` 接口，统一调用方式
2. **统一返回信封**：`{"success": true, "data": {...}}` / `{"success": false, "error": {"code": "...", "message": "..."}}`
3. **自描述 Schema**：每个工具直接提供 `input_schema()`，消除 `tool_schemas.json`
4. **自动化注册**：注释驱动代码生成，新增工具只需创建 `.hpp` 文件
5. **组合优于继承**：通过 `Capabilities` 声明式标记能力（needs_scene / needs_node），而非继承链
6. **分类体系**：两轴分类——`source` 决定可见性规则，`category` 决定展示归类
7. **YAML 驱动测试**：通过 YAML 文件编写测试用例，无须重复样板代码

## 架构设计

### 工具分类体系

每个工具同时属于两个独立的分类维度：

```
         源（source）
         meta  │  built_in  │  custom
                │             │
 "scene"  ──────┼─────────────┼──────────
                │ create_scene │ custom_scene_helper
 分  "node"  ───┼─────────────┼──────────
 类               │ set_property │ custom_node_util
                │             │
(category) "editor" ─┼─────────────┼──────────
                │ play_scene  │ custom_editor_tool
                │             │
```

| 维度 | 字段 | 含义 | 影响 |
|------|------|------|------|
| 源 | `source()` | `"meta"` / `"built_in"` / `"custom"` | 可见性规则、`custom_` 前缀 |
| 分组 | `category()` | 自由字符串（如 `"scene"`） | `list_tool_categories` 自动归类 |

### ITool 接口

```cpp
class ITool {
public:
    virtual ~ITool() = default;

    // ── 元数据 ──
    virtual String name() const = 0;
    virtual String brief() const = 0;
    virtual String description() const = 0;
    virtual Dictionary input_schema() const = 0;

    // ── 分类（所有工具必须指定）──
    virtual String category() const = 0;              // 分组 key
    virtual String category_label() const {           // 分组展示名（可自定义）
        return category();
    }

    // ── 源标记 ──
    virtual String source() const { return "built_in"; }

    // ── 能力声明（组合优于继承）──
    virtual bool needs_scene() const { return false; }
    virtual bool needs_node() const { return false; }

    // ── 统一入口（非虚模板方法）──
    Dictionary execute(const Dictionary &args);

    // ── 注册名计算 ──
    String registered_name() const {
        if (source() == "custom") return "custom_" + name();
        return name();
    }

protected:
    virtual Dictionary execute_impl(const ToolContext &ctx) = 0;
};
```

### ToolResult — 统一返回信封

```cpp
class ToolResult {
public:
    static Dictionary ok(Dictionary data = {});
    // → {"success": true, "data": { ... }}

    static Dictionary err(String code, String message);
    // → {"success": false, "error": {"code": "...", "message": "..."}}

    static bool is_ok(const Dictionary &r);
    static bool is_err(const Dictionary &r);
};
```

`ITool::execute()` 的模板方法自动确保返回结果符合此信封。

### ToolContext — 前置检查上下文

```cpp
struct ToolContext {
    Node *root = nullptr;   // 场景根节点（needs_scene 时）
    Node *node = nullptr;   // 已解析节点（needs_node 时）
    Dictionary args;         // 原始参数
};
```

`ITool::execute()` 按 `capabilities()` 自动执行前置检查，填充 `ToolContext` 后传入 `execute_impl()`。

### HandlerRegistry — 统一存储与调度

```cpp
class HandlerRegistry {
    HashMap<String, std::unique_ptr<ITool>> tools_;
    HashMap<String, CategoryEntry> categories_;

public:
    void register_tool(std::unique_ptr<ITool> tool);   // 统一注册入口
    Dictionary execute(const String &name, const Dictionary &args);  // 统一调度

    // 渐进式披露
    std::vector<ToolInfo> get_always_on_tools();
    std::vector<CategoryInfo> get_categories();
    std::vector<ToolBrief> get_tools_in_category(const String &category);
};
```

内置工具和 SDK 自定义工具存入同一个 `tools_` 哈希表，走同一套调用路径。

### HandlerRegistry::execute() 流程

```
execute(name, args)
  │
  ├─ 1. tools_.find(name) —— 未找到 → ToolResult::err("TOOL_NOT_FOUND")
  │
  ├─ 2. tool->execute(args)
  │      │
  │      ├─ 2a. resolve_context(args)
  │      │    ├─ needs_scene → get_root() → 失败返回 err
  │      │    └─ needs_node  → resolve_node() → 失败返回 err
  │      │
  │      ├─ 2b. execute_impl(ctx) —— 纯业务逻辑
  │      │
  │      └─ 2c. ensure_envelope(result) —— 统一信封包裹
  │
  └─ 3. 返回 Dictionary
```

### 工具实现示例

```cpp
// @tool register
// @source built_in
class SetPropertyTool : public ITool {
    String name() const override { return "set_property"; }
    String category() const override { return "node"; }
    String brief() const override { return "Set a property on a node"; }

    bool needs_scene() const override { return true; }
    bool needs_node() const override { return true; }

    Dictionary input_schema() const override {
        return /* JSON Schema for args */;
    }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        // ctx.root 和 ctx.node 已自动解析
        Node *node = ctx.node;
        String prop = ctx.args["property"];
        Variant value = ctx.args["value"];
        undoable_set(node, prop, value, "Set " + prop);
        return ToolResult::ok({{"property", prop}, {"value", value}});
    }
};
```

对比旧实现——不再有 `get_root()` + `resolve_node()` 的样板代码。

## 代码生成：自动注册

### 注释标记

```cpp
// @tool register
// @source meta
class GodotInfoTool : public ITool { ... };

// @tool register
// @source built_in
class CreateSceneTool : public ITool { ... };
```

### codegen.py 流程

```
扫描 ext/src/built_in/tools/**/*.hpp
  │
  ├─ 匹配 // @tool register 注释
  ├─ 提取 class 名称（class XXXX : public ITool）
  ├─ 提取 // @source <type>
  │
  ▼
生成 generated/generated_registration.cpp
  ├─ #include 所有工具头文件
  └─ register_all_tools() 调用 reg.register_tool<T>()
```

CMake PRE_BUILD 步骤自动执行。

### 新增工具的完整流程

1. 在 `built_in/tools/<category>/` 下创建 `<tool_name>.hpp`
2. 添加 `// @tool register` 和 `// @source <type>` 注释
3. 实现继承 `ITool` 的类
4. 编译——代码生成器自动收集并注册

**无需**修改 `handler_registry.cpp`、`CMakeLists.txt`、`tool_schemas.json`。

## SDK 桥接

### McpToolDefinitionAdapter

SDK 自定义工具通过适配器映射为 `ITool` 接口：

```cpp
class McpToolDefinitionAdapter : public ITool {
    Ref<McpToolDefinition> gdscript_;

    String name() const override { return gdscript_->get("tool_name"); }
    String category() const override { return gdscript_->get("category"); }
    String category_label() const override { return gdscript_->get("category_label"); }
    String source() const override { return "custom"; }

    Dictionary execute_impl(const ToolContext &ctx) override {
        return gdscript_->call("execute", ctx.args);
    }
};
```

GDScript 工具定义：

```gdscript
@tool
class_name AnalysisTool
extends McpToolDefinition

func _init():
    tool_name = "analyze_scene"
    category = "analysis"
    category_label = "自定义分析工具"
    brief = "Analyze current scene structure"

func execute(args: Dictionary) -> Dictionary:
    return ToolResult.ok({"result": "done"})  # SDK 侧也使用 ToolResult
```

### 注册

```gdscript
# 方式 A：继承
AnalysisTool.new().register_tool()

# 方式 B：Callable
McpToolRegistry.get_singleton().register_tool(
    "name", "category", "category_label", "brief", "description",
    schema, callable
)
```

## 文件组织变更

### 当前结构

```
extensions/src/
├── built_in/
│   ├── cmd_utils.hpp/.cpp/.json
│   ├── cmd_info.cpp
│   ├── cmd_meta_tools.cpp
│   ├── node.cpp
│   ├── property.cpp
│   ├── property_3d.cpp
│   ├── collision.cpp
│   ├── find.cpp
│   ├── scene.cpp
│   ├── script_gd.cpp
│   ├── script_cs.cpp
│   ├── script_helpers.cpp
│   ├── project_settings.cpp
│   ├── project_settings_ext.cpp
│   ├── editor_control.cpp
│   ├── input_map.cpp
│   ├── plugin_management.cpp
│   ├── undo.cpp
│   └── search.cpp
├── server/
│   └── registry/handler_registry.hpp/.cpp
└── ...

tools/tool_schemas.json
```

### 目标结构

```
extensions/src/
├── built_in/
│   ├── tool_base.hpp/.cpp          ← ITool, ToolResult, ToolContext
│   ├── tool_macros.hpp             ← 便利宏
│   ├── cmd_utils.hpp/.cpp          ← 保留：resolve_node, undoable_set 等
│   └── tools/                      ← 每个工具独立 .hpp
│       ├── meta/
│       │   ├── godot_info.hpp
│       │   ├── list_tool_categories.hpp
│       │   ├── list_tools.hpp
│       │   ├── get_tool_schema.hpp
│       │   └── call_tool.hpp
│       ├── scene/
│       │   ├── create_scene.hpp
│       │   ├── delete_scene.hpp
│       │   ├── rename_scene.hpp
│       │   ├── branch_to_scene.hpp
│       │   ├── scene_to_branch.hpp
│       │   ├── instantiate_scene.hpp
│       │   ├── open_scene.hpp
│       │   ├── close_scene.hpp
│       │   ├── save_scene.hpp
│       │   ├── save_scene_as.hpp
│       │   ├── save_all_scenes.hpp
│       │   ├── reload_scene.hpp
│       │   ├── get_open_scenes.hpp
│       │   ├── get_open_scene_roots.hpp
│       │   ├── mark_scene_unsaved.hpp
│       │   └── is_scene_dirty.hpp
│       ├── node/
│       │   ├── get_scene_tree.hpp
│       │   ├── get_node_path.hpp
│       │   ├── create_node.hpp
│       │   ├── delete_node.hpp
│       │   ├── rename_node.hpp
│       │   ├── duplicate_node.hpp
│       │   ├── move_node.hpp
│       │   ├── set_property.hpp
│       │   ├── get_property.hpp
│       │   ├── get_property_list.hpp
│       │   ├── batch_set_property.hpp
│       │   ├── attach_script.hpp
│       │   ├── detach_script.hpp
│       │   ├── reset_parent.hpp
│       │   ├── set_as_root.hpp
│       │   ├── add_node_to_group.hpp
│       │   ├── remove_node_from_group.hpp
│       │   ├── set_node_transform_2d.hpp
│       │   ├── set_node_transform_3d.hpp
│       │   ├── get_node_info.hpp
│       │   └── get_script_variables.hpp
│       ├── property/               ← 2D 属性工具
│       ├── property_3d/            ← 3D 属性工具
│       ├── collision/
│       ├── find/
│       ├── script_gd/
│       ├── script_cs/
│       ├── script_helpers/
│       ├── project_settings/
│       ├── project_settings_ext/
│       ├── editor_control/
│       ├── input_map/
│       ├── plugin_management/
│       ├── undo/
│       └── search/
├── sdk/
│   ├── mcp_tool_definition.hpp/.cpp
│   ├── mcp_tool_registry.hpp/.cpp
│   └── mcp_tool_adapter.hpp/.cpp    ← NEW: McpToolDefinitionAdapter
├── server/
│   ├── registry/
│   │   ├── handler_registry.hpp/.cpp ← UPDATED: ITool 统一存储
│   │   └── generated/                ← NEW: 由 codegen.py 生成
│   │       └── generated_registration.cpp
│   └── ...
└── ...

tools/
├── tool_schemas.json                ← DELETE（迁移完成后删除）
└── codegen.py                       ← NEW: 自动注册脚本
```

## YAML 驱动测试

### 测试文件格式

```yaml
# tests/yaml/scene_crud.yaml
name: Scene CRUD

config:
  godot_timeout: 30

setup:
  - tool: create_scene
    args: { path: "res://_test.tscn", root_type: "Node2D" }
    expect: { success: true }
    save: scene_path

steps:
  - tool: open_scene
    args: { scene_path: "${scene_path}" }
    expect: { success: true }

  - tool: create_node
    args: { parent_path: ".", node_type: "Sprite2D" }
    expect: { success: true }
    save: sprite_path

  - tool: set_property
    args: { node_path: "${sprite_path}", property: "position", value: { x: 100, y: 200 } }
    expect: { success: true, "data.value.x": 100 }

  - tool: delete_scene
    args: { path: "res://_test.tscn" }
    expect: { success: true }

teardown:
  - tool: delete_scene
    args: { path: "res://_test.tscn" }
```

### yaml_runner.py

```
YAML 文件 ──► yaml_runner.py ◄── McpSession (httpx)
                │
                ├─ 1. 解析 YAML
                ├─ 2. 连接 Godot（initialize）
                ├─ 3. 逐 step 执行
                │    ├─ 模板替换（${var} → 前序结果）
                │    ├─ 调用 tool
                │    └─ JSONPath 断言
                ├─ 4. teardown 清理
                └─ 5. 生成 JSON + Markdown 报告
```

### 断言机制

```
expect:
  success: true                              # 顶层字段匹配
  "data.path": "res://_test.tscn"            # 点号路径嵌套匹配
  "data.count": { "$gt": 0 }                 # 操作符匹配
```

支持的操作符：`$eq`、`$ne`、`$gt`、`$gte`、`$lt`、`$lte`、`$exists`、`$regex`。

### 新旧测试过渡

- 旧 phase 测试（`tests/test_phases/phase_*.py`）逐步被 YAML 测试覆盖
- YAML 测试完全覆盖后删除旧 phase 文件
- 两个框架在过渡期共存

## 迁移路线

### P1：基础设施（当前阶段）

**文件**：

| 文件 | 操作 | 说明 |
|------|------|------|
| `built_in/tool_base.hpp` | 新建 | `ITool`、`ToolResult`、`ToolContext`、`Capabilities` |
| `built_in/tool_base.cpp` | 新建 | `resolve_context()`、`ensure_envelope()` 实现 |
| `tools/codegen.py` | 新建 | 注释扫描 + 注册代码生成 |
| `server/registry/handler_registry.hpp` | 修改 | 新增 `register_tool(unique_ptr<ITool>)` 方法 |
| `server/registry/handler_registry.cpp` | 修改 | 实现统一注册、自动归类、渐进式披露 |
| `extensions/CMakeLists.txt` | 修改 | GLOB 源文件 + codegen PRE_BUILD 步骤 |
| `agdsdk/mcp_tool_adapter.hpp` | 新建 | SDK ↔ ITool 桥接 |

**验证**：`cmake --build build` 通过，现有工具完全不受影响。

### P2：迁移 meta 组（5 个工具）

**迁移文件**：

| 旧文件 | 新文件 |
|--------|--------|
| `cmd_info.cpp` → | `tools/meta/godot_info.hpp` |
| `cmd_meta_tools.cpp` → | `tools/meta/list_tool_categories.hpp`、`list_tools.hpp`、`get_tool_schema.hpp`、`call_tool.hpp` |

**验证**：渐进式披露正常工作，`tools/list` 返回 5 个 meta 工具。

### P3：迁移 scene 组（16 个工具）

**迁移文件**：

`scene.cpp` → 拆分为 `tools/scene/*.hpp`（16 个文件）。

**首批 YAML 测试**：`tests/yaml/scene_crud.yaml`。

**验证**：`yaml_runner.py tests/yaml/scene_crud.yaml` 全部通过。

### P4：迁移 node + property 组（~44 个工具）

**迁移文件**：

`node.cpp` → `tools/node/*.hpp`（21 个文件）
`property.cpp` → `tools/property/*.hpp`（21 个文件）
`property_3d.cpp` → `tools/property_3d/*.hpp`（6 个文件）
`collision.cpp` → `tools/collision/*.hpp`（2 个文件）

**验证**：needs_node / needs_scene 能力声明消除样板代码。全 YAML 测试通过。

### P5：迁移剩余全部工具（~54 个工具）

**迁移文件**：

`find.cpp`、`script_gd.cpp`、`script_cs.cpp`、`script_helpers.cpp`、`project_settings.cpp`、`project_settings_ext.cpp`、`editor_control.cpp`、`input_map.cpp`、`plugin_management.cpp`、`undo.cpp`、`search.cpp`

**验证**：全部 119 个工具迁移完成，`register_all_tools()` 完全由 `generated_registration.cpp` 驱动。

### P6：清理 + 文档

**操作**：

1. 删除 `tools/tool_schemas.json`
2. 删除全部旧 `cmd_*.cpp` 文件
3. 删除旧 `register_<group>()` 前向声明和调用
4. 删除旧 phase 测试文件
5. 用 YAML 测试覆盖全部工具
6. 更新 `.repo_wiki/` 文档反映新架构

## YAML 测试文件计划

| 测试文件 | 覆盖范围 | 关联阶段 |
|----------|----------|----------|
| `meta_tools.yaml` | 5 个 meta 工具基本功能 | P2 |
| `scene_crud.yaml` | 场景创建/删除/保存/打开 | P3 |
| `scene_branch.yaml` | 分支到场景/场景到分支 | P3 |
| `node_crud.yaml` | 节点创建/删除/重命名/移动/复制 | P4 |
| `node_property.yaml` | 属性获取/设置/批量设置 | P4 |
| `property_2d.yaml` | 2D 位置/旋转/缩放/可见/颜色 | P4 |
| `property_3d.yaml` | 3D 位置/旋转/缩放 | P4 |
| `collision.yaml` | 碰撞形状添加 | P4 |
| `find_nodes.yaml` | 4 种查找方式 | P5 |
| `gdscript.yaml` | GDScript CRUD + 验证 | P5 |
| `csharp_script.yaml` | C# 脚本 CRUD + 构建 | P5 |
| `script_helpers.yaml` | call_method / get_variable / set_variable | P5 |
| `project_settings.yaml` | 项目设置 + 扩展设置 | P5 |
| `editor_control.yaml` | 播放/停止/刷新/信息 | P5 |
| `input_map.yaml` | 输入动作管理 | P5 |
| `plugin_management.yaml` | 插件列表/启用/禁用 | P5 |
| `undo_redo.yaml` | 撤销/重做 | P5 |
| `search.yaml` | 文件查找/项目搜索/替换 | P5 |
