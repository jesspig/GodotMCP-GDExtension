# 阶段系统

> 18 个阶段文件，每个导出 `TOOL_TESTS: list[ToolTest]`，测试 GodotMCP 工具的一个功能领域。

## 数据类

```python
@dataclass
class ToolTest:
    name: str                           # 工具名，用于匹配 tools/list
    run: Callable[[TestContext], Awaitable[TestResult]]
    setup: Optional[Callable] = None    # 测试前准备（可选）
    teardown: Optional[Callable] = None # 测试后清理（可选）
    requires_dotnet: bool = False       # 需要 dotnet CLI

@dataclass
class TestResult:
    tool: str
    status: str               # PASS / FAIL / SKIP
    expected: Any = None
    actual: Any = None
    error: Optional[str] = None
    disk_verified: bool = False
    disk_detail: Optional[str] = None
    duration: float = 0.0

@dataclass
class PhaseReport:
    name: str
    results: list[TestResult] = field(default_factory=list)
    start_time: Optional[float] = None
    end_time: Optional[float] = None
```

## PhaseRunner 执行逻辑

```mermaid
flowchart TD
    A[开始阶段] --> B[遍历 TOOL_TESTS]
    B --> C{工具名在<br/>available_tools 中?}
    C -->|否| D[SKIP: 工具未发现]
    C -->|是| E{requires_dotnet<br/>且 dotnet 不可用?}
    E -->|是| F[SKIP: dotnet 不可用]
    E -->|否| G[运行 setup（可选）]
    G --> H[执行 tool.run(ctx)]
    H --> I[记录 duration]
    I --> J[运行 teardown（可选）]
    J --> K[收集 TestResult]
    K --> B
    B --> L[完成阶段]
```

- 按 `TOOL_TESTS` 列表顺序执行
- 每个测试的 `setup` 和 `teardown` 是当前 `ToolTest` 实例的属性（不是列表级别的）
- 异常被捕获并报告为 `FAIL`（含 traceback）
- `TestContext` 在阶段间共享，包含会话引用、可用工具集合、创建的文件/节点列表等状态

## 阶段一览

| 阶段 | 文件 | 测试内容 | 工具数 |
|------|------|----------|--------|
| 01 | `phase_01_connect` | 连接检测（ping、engine/plugin version、editor info） | 4 |
| 02 | `phase_02_net_check` | get_editor_info（已合并到 01） | 0 |
| 03 | `phase_03_read_only` | 只读查询（scene tree、settings、autoloads、plugins 等） | 17 |
| 04 | `phase_04_scene_node` | 场景/节点 CRUD（创建、删除、重命名、复制、移动） | 15 |
| 05 | `phase_05_property_2d` | 2D 属性读写（position、rotation、scale、visible 等） | — |
| 06 | `phase_06_property_3d` | 3D 属性读写 | — |
| 07 | `phase_07_collision` | 碰撞体添加（circle、rectangle） | — |
| 08 | `phase_08_find` | 节点搜索（name、type、group、script） | — |
| 09 | `phase_09_script` | GDScript 创建/编辑/验证 | — |
| 10 | `phase_10_scene_mgmt` | 场景文件管理（save、open、close、branch 等） | — |
| 11 | `phase_11_search` | 文件搜索与替换 | — |
| 12 | `phase_12_input` | InputMap 操作（添加/移除 action、设置事件） | — |
| 13 | `phase_13_project_write` | 项目设置写入 | — |
| 14 | `phase_14_editor_control` | play/stop/is_playing、refresh | — |
| 15 | `phase_15_plugin` | 插件管理（list、enable/disable） | — |
| 16 | `phase_16_undo` | undo/redo | — |
| 17 | `phase_17_csharp` | C# 脚本操作（需要 dotnet） | — |
| 18 | `phase_18_cleanup` | 清理所有创建的文件、恢复备份 | 1 |
