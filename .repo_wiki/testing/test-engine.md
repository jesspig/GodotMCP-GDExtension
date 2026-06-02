# C++ 测试引擎

> 进程内测试引擎，通过 `POST /run-tests`（HTTP）或 `TestRunnerDock`（C++ 面板）接收 YAML 配置文件，自动编排并执行测试，完成后清理产物。

## 架构

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

**关键设计**：`TestEngine` 是普通 C++ 类（非 `Object` 子类），不经过 MCP 协议层，避免"工具测试工具"的套娃问题。

## 入口

| 入口 | 调用方式 | 适用场景 |
|------|----------|----------|
| `POST /run-tests` | HTTP（`HttpServer` 路由） | Python 编排器 / CI 脚本 / `curl` |
| `TestRunnerDock` | C++ 直接调用 `TestEngine::run()` | 编辑器底部面板 |
| `TestEngine::run()` | C++ API | 程序化调用 |

**三种入口共享同一套 `TestEngine` 实现、断言引擎、磁盘校验**。

### HTTP 接口

```http
POST /run-tests HTTP/1.1
Content-Type: application/x-yaml

name: scene_test
before_all:
  - tool: create_scene
    args: { name: "test", root_type: "Node2D" }
after_all:
  - tool: close_scene
    args: {}
tests:
  - tool: create_node
    description: "创建 Node2D"
    args: { type: "Node2D", name: "test_node" }
    expect:
      status: success
```

```json
HTTP/1.1 200 OK
Content-Type: application/json

{
  "summary": { "total": 1, "passed": 1, "failed": 0, "warnings": 0 },
  "results": [
    {
      "tool": "create_node",
      "description": "创建 Node2D",
      "status": "PASS",
      "duration_ms": 12.3
    }
  ],
  "cleanup": {
    "deleted": ["res://scenes/test.tscn"],
    "cleaned_dirs": ["res://scenes/"],
    "preserved_user_files": [],
    "warnings": []
  }
}
```

## 执行流程

```
TestEngine::run(yaml_content)
  │
  ├─ 1. ryml::parse(YAML) → TestSuite
  │      ├─ name / description
  │      ├─ before_all:  ToolStep[]
  │      ├─ before_each:  ToolStep[]
  │      ├─ after_each:   ToolStep[]
  │      ├─ after_all:    ToolStep[]
  │      └─ tests:        TestCase[]
  │
  ├─ 2. EditorFileSystem 快照 (纯内存遍历)
  │     EditorFileSystem::get_filesystem() → root
  │     collect_all_files(root) → before_files: HashSet<String>
  │
  ├─ 3. before_all 链
  │     for step in suite.before_all:
  │       result = HandlerRegistry::execute(step.tool, step.args)
  │       extract_paths(result) → tracked_paths
  │
  ├─ 4. 测试循环
  │     for each test in suite.tests:
  │     │
  │     ├─ before_each 链
  │     │   for step in suite.before_each:
  │     │     result = HandlerRegistry::execute(step.tool, step.args)
  │     │     extract_paths(result) → tracked_paths
  │     │
  │     ├─ result = HandlerRegistry::execute(test.tool, test.args)
  │     ├─ extract_paths(result) → tracked_paths
  │     │
  │     ├─ 断言引擎校验 result vs test.expect
  │     │   ├─ check_status(result, expect.status)
  │     │   ├─ check_has_keys(result, expect.has_keys)
  │     │   ├─ check_field_checks(result, expect.field_checks)
  │     │   └─ check_error_contains(result, expect.error_contains)
  │     │
  │     ├─ 磁盘校验 (test.disk_verify.enabled)
  │     │   ├─ verify_scene_file(path, nodes)      → file_errors
  │     │   ├─ verify_project_godot(key, expect)    → file_errors
  │     │   └─ verify_raw_text(path, contains)      → file_errors
  │     │
  │     ├─ after_each 链
  │     │   for step in suite.after_each:
  │     │     result = HandlerRegistry::execute(step.tool, step.args)
  │     │     extract_paths(result) → tracked_paths
  │     │
  │     └─ 记录 TestResult
  │
  ├─ 5. after_all 链
  │     for step in suite.after_all:
  │       result = HandlerRegistry::execute(step.tool, step.args)
  │       extract_paths(result) → tracked_paths
  │
  ├─ 6. EditorFileSystem 快照 → after_files
  │
  ├─ 7. 清理
  │     diff = after_files - before_files
  │     to_delete = diff ∩ tracked_paths    (双源确认)
  │     to_warn   = diff - tracked_paths    (用户手动创建, 不删)
  │     for f in to_delete:
  │       DirAccess::remove_absolute(f)
  │     for 父目录 (自底向上, 去重):
  │       if 为空 && dir != "res://" && dir != "res://addons/"
  │         → DirAccess::remove_absolute(dir)
  │
  └─ 8. 返回 { summary, results, cleanup }
```

## YAML 配置格式

### 完整格式

```yaml
name: property_tests
description: 属性操作完整验证

before_all:
  - tool: create_scene
    args: { name: "test_scene", root_type: "Node2D" }

before_each: []

after_each:
  - tool: delete_node
    args: { node: "tmp" }

after_all:
  - tool: close_scene
    args: {}

tests:
  # ── 创建节点 + 存在性校验 ──
  - tool: create_node
    description: 创建 Node2D，校验 tscn
    args: { type: "Node2D", name: "test_node", parent: "." }
    expect:
      status: success
    disk_verify:
      scene:
        path: "res://scenes/test_scene.tscn"
        nodes:
          - path: "test_node"
            type: "Node2D"
            exists: true

  # ── 设 position + 磁盘回读校验 ──
  - tool: set_property
    description: position → (100,100)
    args: { node: "test_node", property: "position", value: [100, 100] }
    expect:
      status: success
    disk_verify:
      scene:
        path: "res://scenes/test_scene.tscn"
        nodes:
          - path: "test_node"
            properties:
              - path: "position"
                expect: [100, 100]
                type: Vector2
                tolerance: 0.001

  # ── 删除 + 不存在校验 ──
  - tool: delete_node
    description: 删除节点
    args: { node: "test_node" }
    expect:
      status: success
    disk_verify:
      scene:
        path: "res://scenes/test_scene.tscn"
        nodes:
          - path: "test_node"
            exists: false

  # ── 错误测试 ──
  - tool: create_node
    description: 非法类型应报错
    args: { type: "NonExistentType" }
    expect:
      status: error
      error_contains: "Unknown"

  # ── 项目设置校验 ──
  - tool: set_project_setting
    description: 改项目名
    args: { setting: "application/config/name", value: "TestGame" }
    expect:
      status: success
    disk_verify:
      project_godot:
        - key: "application/config/name"
          expect: "TestGame"

  # ── 原始文本校验 ──
  - tool: set_property
    description: visible=false
    args: { node: "test_node", property: "visible", value: false }
    expect:
      status: success
    disk_verify:
      raw_text:
        - path: "res://scenes/test_scene.tscn"
          contains: "visible = false"
```

### 数据结构

```cpp
// ── 生命周期步骤 ──
struct ToolStep {
    String tool;
    Dictionary args;
};

// ── 断言 ──
struct FieldCheck {
    String key;           // 点分隔路径, "data.node_path"
    String type;          // "string"|"int"|"float"|"bool"|"list"|"dict"|"any"
    Variant value;        // 可选精确值
    bool not_empty;       // 选项非空
};

struct TestExpectation {
    String status;                     // "success"|"error"
    Vector<String> has_keys;
    Vector<FieldCheck> field_checks;
    String error_contains;
};

// ── 磁盘校验 ──
struct ScenePropertyCheck {
    String path;            // "position", "modulate", "material/0/albedo_color"
    Variant expect;         // 期望值 (YAML 原始形式)
    String type;            // 必须显式: "Vector2"|"Vector3"|"Color"|"float"|"int"|"bool"|"String"
    float tolerance = -1;   // -1 = 使用全局
};

struct SceneNodeCheck {
    String path;            // 节点名称, 相对场景根
    String type;            // "Node2D", "Sprite2D"...
    bool exists = true;
    Vector<ScenePropertyCheck> properties;
};

struct DiskVerifyConfig {
    bool enabled = false;
    float tolerance = 0.0001;
    String scene_path;
    Vector<SceneNodeCheck> scene_nodes;
    Vector<KeyExpectPair> project_godot;
    Vector<RawTextCheck> raw_text;
};

// ── 测试用例 ──
struct TestCase {
    String tool;
    String description;
    Dictionary args;
    bool skip_if_missing = true;
    TestExpectation expect;
    DiskVerifyConfig disk_verify;
};

struct TestSuite {
    String name, description;
    Vector<ToolStep> before_all, before_each, after_each, after_all;
    Vector<TestCase> tests;
};
```

## 磁盘校验引擎

### 支持的校验类型

| 类型 | Godot API | 说明 |
|------|-----------|------|
| `.tscn` | `ResourceLoader::load` → `PackedScene` → `instantiate()` | 遍历节点树, `node.get(path)`, 类型转换后容差比较 |
| `.tres` | `ResourceLoader::load` → `Resource` | 同上 |
| `project.godot` | `ConfigFile::load` → `get_value()` | 逐 key 比较 |
| 任意文件 | `FileAccess::get_file_as_string()` | 子串包含/不包含 |

### 类型转换与容差

通过 `type` 字段将 YAML 原始值转为正确的 Godot Variant 再比较：

| type | YAML → Variant | 比较方式 |
|------|----------------|----------|
| `Vector2` | `[100, 100]` → `Vector2(100, 100)` | 向量距离 ≤ tolerance |
| `Vector3` | `[1, 2, 3]` → `Vector3(1, 2, 3)` | 向量距离 ≤ tolerance |
| `Color` | `[1, 0, 0, 1]` → `Color(1, 0, 0, 1)` | 四通道距离 ≤ tolerance |
| `float` | `3.14` → `float(3.14)` | `\|a-e\|` ≤ tolerance |
| `int` | `42` → `int(42)` | 精确等于 |
| `bool` | `true` → `bool(true)` | 精确等于 |
| `String` | `"hello"` → `String("hello")` | 精确等于 |
| `Rect2` | `[[0,0],[100,100]]` → `Rect2(0,0,100,100)` | position + size 分别校验 |

`type` **必须显式声明**，不会自动推断。

### 清理策略：双源追踪

| 来源 | 内容 | 用途 |
|------|------|------|
| EditorFileSystem 快照差分 | 所有新增文件 | 不漏任何文件 |
| 工具返回值路径追踪 | `result["path"]`、`result["scene_path"]` 等 | 区分测试/用户文件 |

**规则**：只删除 `diff ∩ tracked_paths` 中的文件。`diff - tracked_paths` 中的文件视为用户手动创建，报告 WARN 但**不删除**。

**路径提取**：从每个 `HandlerRegistry::execute()` 返回的 Dictionary 中，递归查找以 `res://` 开头的字符串值（`path`、`scene_path`、`file_path`、`script_path`、`resource_path` 等 key）。

## 文件结构

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

## TestRunnerDock UI

C++ 底部面板，使用 `Object::call("add_control_to_bottom_panel", dock, "MCP Tests")` 添加（因 godot-cpp 10.0.0-rc1 未绑定此方法）。

布局：

```
┌──────────────────────────────────────────────────┐
│ [tests/scene_test.test.yaml ▼] [▶ 运行] [🔄]     │  ← HBoxContainer (toolbar)
├──────────────────────────────────────────────────┤
│ Test Name       │ Tool         │ Status │ Detail  │  ← Tree (4 列)
│──────────────────────────────────────────────────│
│ ✔ 创建 Node2D   │ create_node  │ PASS   │         │
│ ✗ 设 position   │ set_property │ FAIL   │ 展开 ▼  │
│                  │              │        │ 磁盘:   │
│                  │              │        │ 期望    │
│                  │              │        │ (100,100)│
│                  │              │        │ 实际    │
│                  │              │        │ (0,0)   │
│ ✔ 删除节点      │ delete_node  │ PASS   │         │
├──────────────────────────────────────────────────┤
│ ✅ 4/5 passed  (1 failed)  耗时 1.2s            │  ← Label (summary)
└──────────────────────────────────────────────────┘
```

组件：
- `OptionButton`：扫描 `*.test.yaml` 文件
- `Button "▶ 运行"`：触发测试
- `Button "🔄"`：刷新文件列表
- `Tree`：4 列结果展示，FAIL 项可展开
- `Label`：汇总统计

## 外部 Python 测试 (CI/开发)

```bash
# 启动 Godot 后:
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/scene.test.yaml

# 或用编排器 (管理 Godot 生命周期):
uv run python tests/test_orchestrator.py
```

旧 `tests/test_phases/` Python 阶段文件与新 YAML 测试在过渡期共存，最终被 YAML 替代。
