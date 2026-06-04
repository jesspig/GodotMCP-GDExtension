# 阶段系统（已废弃——历史参考）

> **状态：已删除。** 18 个 `tests/test_phases/phase_XX_*.py` 文件已于 2026-06 的清理阶段 1 中删除。
> 唯一测试路径为 `tests/test_orchestrator.py` 调 `/run-tests` 端点，由 C++ `TestEngine` 执行 YAML 驱动测试。
>
> **保留此文档仅作为历史参考**，新工具/新测试不应再使用 `ToolTest`/`PhaseRunner` 数据类。

## 数据类（已删除）

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

## 旧 PhaseRunner 执行流程

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

## 阶段一览（历史）

| 阶段 | 文件 | 测试内容 |
|------|------|----------|
| 01 | `phase_01_connect` | 连接检测（ping、engine/plugin version、editor info） |
| 02 | `phase_02_net_check` | get_editor_info（已合并到 01） |
| 03 | `phase_03_read_only` | 只读查询（scene tree、settings、autoloads、plugins 等） |
| 04 | `phase_04_scene_node` | 场景/节点 CRUD |
| 05 | `phase_05_property_2d` | 2D 属性读写（已被 `property_2d.yaml` 替代） |
| 06 | `phase_06_property_3d` | 3D 属性读写（已被 `property_3d.yaml` 替代） |
| 07 | `phase_07_collision` | 碰撞体添加 |
| 08 | `phase_08_find` | 节点搜索 |
| 09 | `phase_09_script` | GDScript 创建/编辑/验证 |
| 10 | `phase_10_scene_mgmt` | 场景文件管理 |
| 11 | `phase_11_search` | 文件搜索与替换 |
| 12 | `phase_12_input` | InputMap 操作 |
| 13 | `phase_13_project_write` | 项目设置写入 |
| 14 | `phase_14_editor_control` | play/stop/is_playing、refresh |
| 15 | `phase_15_plugin` | 插件管理（list、enable/disable） |
| 16 | `phase_16_undo` | undo/redo |
| 17 | `phase_17_csharp` | C# 脚本操作（需要 dotnet） |
| 18 | `phase_18_cleanup` | 清理所有创建的文件、恢复备份 |

## 迁移路径

旧 Python 阶段文件的所有测试覆盖能力在 C++ `TestEngine` 中通过 YAML 形式重新表达：

| 旧 phase 范围 | 新 YAML 测试 |
|--------------|-------------|
| 01-04（连接/只读/场景CRUD） | `tests/yaml_tests/scene_basic.yaml`、`tests/yaml_tests/scene_lifecycle.yaml` |
| 05-06（2D/3D 属性） | `tests/yaml_tests/property_2d.yaml`、`tests/yaml_tests/property_3d.yaml`（已迁移到节点属性工具） |
| 07-08（碰撞/查找） | `tests/yaml_tests/collision.yaml`、`tests/yaml_tests/find_node.yaml` |
| 09（脚本） | `tests/yaml_tests/script_*.yaml` |
| 10-15（场景/搜索/输入/项目设置/编辑器控制/插件） | `tests/yaml_tests/<area>.yaml` |
| 16（undo） | `tests/yaml_tests/undo.yaml` |
| 17（csharp） | `tests/yaml_tests/csharp.yaml` |
| 18（cleanup） | 由 `TestEngine` 自动化处理（双源追踪） |

详见 [testing/test-engine.md](test-engine.md) 与 [testing/orchestrator.md](orchestrator.md)。
