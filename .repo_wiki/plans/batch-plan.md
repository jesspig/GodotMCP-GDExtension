# 批次执行方案 — 并行/串行调度

## 总览

```
Batch 0 (19 nodes, 5 subagents)  ──→  GATE
Batch 1 (5 nodes, 4 subagents)   ──→  GATE
Batch 2 (5 nodes, 4 subagents)   ──→  GATE
Batch 3 (12 nodes, 5 subagents)  ──→  GATE
Batch 4 (7 nodes, 4 subagents)   ──→  GATE
Batch 5 (3 nodes, 3 subagents)   ──→  GATE
Batch 6 (4 nodes, 3 subagents)   ──→  GATE
Batch 7 (2 nodes, 2 subagents)   ──→  FINAL GATE
```

---

## Batch 0 — 基础修复 + 关键 Bug + 基础设施创建

> **目标**：修复所有 P0 bug，建立共享基础设施，清理构建系统
> **并行度**：5 subagents
> **预估耗时**：30-40 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-0A** | B1,B2,B3,B4,B5,B7,B10 | `CMakeLists.txt` (根), `ext/CMakeLists.txt` | ~80 行改动 |
| **SA-0B** | B6,B9 | `CMakeLists.txt` (根 L68-92), `build.py` | ~60 行改动 |
| **SA-0C** | X1,X2,X3,X4 | `mcp_console.cpp`, `bridge.cpp`, `game_bridge.cpp`, `http_server.cpp` | ~80 行改动 |
| **SA-0D** | I1,I2,I3,I4 | `cmd_utils/` (4 个新文件), `tool_base.hpp` | ~200 行新增 |
| **SA-0E** | T4,T5,T7 | `tools/**/workspace/debugger_*.hpp`, `set_workspace_*.hpp`, `register_existing.hpp`, `register_meta.hpp` | ~350 行删除 |

### 文件域验证（无冲突）

```
SA-0A: CMakeLists.txt, ext/CMakeLists.txt
SA-0B: CMakeLists.txt(根L68-92独立段), build.py           ← 注意：与 SA-0A 共享根 CMakeLists.txt，但 SA-0A 改 L7+L191+L44+L34+L157+L110，SA-0B 改 L68-92，行范围不重叠
SA-0C: mcp_console.cpp, bridge.cpp, game_bridge.cpp, http_server.cpp
SA-0D: cmd_utils/memdelete_guard.hpp(新), cmd_utils/schema_builder.hpp(新), cmd_utils/error_codes.hpp(新), tool_base.hpp
SA-0E: tools/**/workspace/debugger_*.hpp, set_workspace_*.hpp, register/register_existing.hpp, register/register_meta.hpp
```

> **SA-0A / SA-0B 冲突处理**：二者都修改根 `CMakeLists.txt`。SA-0A 改 L7（RELEASE option）和 ext/CMakeLists.txt 多处；SA-0B 改 L68-92（file WRITE → configure_file）。行范围不重叠，但为安全起见，**SA-0A 先完成根 CMakeLists.txt 改动后，SA-0B 再改**。或者将根 CMakeLists.txt 全部交给 SA-0A，SA-0B 只做 build.py。

**推荐调整**：将 B6 也交给 SA-0A（根 CMakeLists.txt 全部改动集中一人），SA-0B 只做 B9（build.py）。

### GATE 0 验证

```bash
# 1. 编译验证（Debug + Unity ON）
uv run python build.py

# 2. 编译验证（Unity OFF — 验证 include 完整性）
uv run python build.py --clean
# 临时修改 ext/CMakeLists.txt: GODOTMCP_UNITY_BUILD=OFF
# 或命令行：cmake -DGODOTMCP_UNITY_BUILD=OFF ...

# 3. 运行测试（需 Godot 编辑器运行）
pytest tests/test_orchestrator.py -v --asyncio-mode=auto
```

---

## Batch 1 — 构建完善 + 工具清理 + 测试修复

> **目标**：完善构建配置，合并 args_get，修复测试基础设施
> **并行度**：4 subagents
> **预估耗时**：15-20 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-1A** | B8 | `ext/CMakeLists.txt` L177,179 | ~10 行 |
| **SA-1B** | B10 | `ext/CMakeLists.txt` (新增 1 行 SKIP_UNITY_BUILD) | ~3 行 |
| **SA-1C** | T6 | `cmd_utils.hpp`, `cmd_utils/args_get_typed.hpp` | ~80 行改动 |
| **SA-1D** | U3,U4 | `tests/test_orchestrator.py`, `tests/godot_manager.py` | ~60 行 |

### 文件域验证

```
SA-1A: ext/CMakeLists.txt (L177,179 警告标志段)
SA-1B: ext/CMakeLists.txt (新增行，与 SA-1A 不重叠)    ← 合并到 SA-1A
SA-1C: cmd_utils.hpp, cmd_utils/args_get_typed.hpp
SA-1D: tests/test_orchestrator.py, tests/godot_manager.py
```

> **优化**：SA-1A + SA-1B 合并为一个 subagent（同一文件）。实际并行度 = 3 subagents。

### GATE 1 验证

```bash
uv run python build.py
# 验证警告级别提升后无新 warning
# 验证 args_get 合并后现有工具编译正常
```

---

## Batch 2 — 工具安全迁移 + 架构清理

> **目标**：修复所有权 bug + 应用 RAII guard + 死代码清理 + logger 修复
> **并行度**：4 subagents
> **预估耗时**：30-40 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-2A** | X5,T1 | `tools/**/*.hpp` (~20 文件：create_mesh_instance_3d, create_collision_shape, create_audio_player 等) | ~200 行改动 |
| **SA-2B** | A4,M5 | `tool_executor.hpp/cpp`, `handler_registry.cpp` (L65-69), `tool_base.hpp` (L42-45) | ~60 行删除 |
| **SA-2C** | U1,P5 | `ui/mcp_logger.cpp`, `ui/mcp_logger.hpp` | ~80 行改动 |
| **SA-2D** | M3 | 3 个文件 (#pragma 顺序) | ~6 行 |

### 文件域验证

```
SA-2A: tools/**/create_*.hpp, tools/**/add_*.hpp, tools/**/duplicate_*.hpp 等 (~20 文件)
SA-2B: tool_executor.hpp, tool_executor.cpp, handler_registry.cpp (L65-69), tool_base.hpp (L42-45)
SA-2C: mcp_logger.cpp, mcp_logger.hpp
SA-2D: fallback_tools.hpp, save_scene.hpp, delete_file.hpp
```

> **注意**：SA-2B 触碰 `tool_base.hpp`，而 Batch 0 的 SA-0D 也改过此文件（添加空 schema 默认实现）。不冲突——SA-0D 添加内容，SA-2B 删除内容（L42-45），行范围不同。

### GATE 2 验证

```bash
uv run python build.py
# 重点验证：
# 1. MemdeleteGuard 在所有工具中正确编译
# 2. 无 memdelete 遗漏或双重释放
# 3. Logger 持久化句柄正常工作
```

---

## Batch 3 — 架构重构 + 性能优化 (第一批)

> **目标**：核心架构去重 + 性能优化 + 功能修复
> **并行度**：5 subagents
> **预估耗时**：40-50 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-3A** | A1,A2,A3 | `handler_registry.hpp/cpp`, `mcp_handler.hpp/cpp` | ~150 行改动 |
| **SA-3B** | A5,A6 | `tool_executor.hpp/cpp`, `bridge.cpp` | ~60 行改动 |
| **SA-3C** | A7,P2 | `prompt_provider.cpp`, `http_connection.cpp` | ~80 行改动 |
| **SA-3D** | P4,P7 | 多文件 (~10 处 reserve), `get_performance_monitors.hpp` | ~60 行改动 |
| **SA-3E** | P8,M4 | `get_console_errors.hpp`, `get_console_warnings.hpp`, `set_shader_uniform.hpp` | ~80 行改动 |

### 文件域验证

```
SA-3A: handler_registry.hpp, handler_registry.cpp, mcp_handler.hpp, mcp_handler.cpp
SA-3B: tool_executor.hpp, tool_executor.cpp, bridge.cpp
SA-3C: prompt_provider.cpp, http_connection.cpp
SA-3D: 多文件 reserve() + get_performance_monitors.hpp
SA-3E: get_console_errors.hpp, get_console_warnings.hpp, set_shader_uniform.hpp
```

> **注意**：SA-3B 触碰 `tool_executor.cpp`，而 Batch 2 的 SA-2B 也改过此文件。不冲突——SA-2B 删除死代码（logger_），SA-3B 清理 auth_token 参数。但需确保 SA-2B 完成后再开始 SA-3B。

### GATE 3 验证

```bash
uv run python build.py
pytest tests/test_orchestrator.py -v --asyncio-mode=auto
# 重点验证：
# 1. HandlerRegistry 去重后工具搜索/分类仍正常
# 2. 场景树深度限制不影响正常场景
# 3. set_shader_uniform undo 支持正常工作
```

---

## Batch 4 — 性能优化 (第二批) + 工具合并

> **目标**：剩余性能优化 + 工具合并
> **并行度**：4 subagents
> **预估耗时**：30-40 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-4A** | P1,P3 | `handler_registry.cpp` (搜索索引), `tool_executor.cpp` (日志格式化) | ~100 行改动 |
| **SA-4B** | P6,U2,U5 | `game_bridge.cpp`, `mcp_dock.cpp`, `yaml_parser.hpp` | ~60 行改动 |
| **SA-4C** | M6,M7 | `get_fps.hpp`, `get_memory_usage.hpp`, `get_object_count.hpp`, `get_physics_stats.hpp`, `get_render_stats.hpp`, `get_console_errors.hpp`, `get_console_warnings.hpp` | ~200 行删除 |
| **SA-4D** | M8 | `get_debugger_errors.hpp`, `get_debugger_state.hpp`, `get_debugger_status.hpp` | ~80 行删除 |

### 文件域验证

```
SA-4A: handler_registry.cpp, tool_executor.cpp
SA-4B: game_bridge.cpp, mcp_dock.cpp, yaml_parser.hpp
SA-4C: tools/**/workspace/get_fps/memory/object/physics/render.hpp, get_console_errors/warnings.hpp
SA-4D: tools/**/workspace/get_debugger_*.hpp
```

> **注意**：SA-4A 触碰 `handler_registry.cpp`，SA-3A 也改过此文件。依赖关系 A1→P1 已保证顺序。SA-4A 触碰 `tool_executor.cpp`，SA-3B 也改过。依赖关系 A5→P3 已保证顺序。

### GATE 4 验证

```bash
uv run python build.py
# 重点验证：
# 1. 搜索索引正确性
# 2. 合并后的工具功能等价
# 3. 性能基准（可选）
```

---

## Batch 5 — Schema 大规模迁移 + 模板化

> **目标**：将 SchemaBuilder 应用到所有 171 个工具 + 提取 Resource/Toggle 模板
> **并行度**：3 subagents
> **预估耗时**：40-50 分钟（T2 是大工程）

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-5A** | T2 (前半) | `tools/meta/`, `tools/editor_tools/scene_tree/`, `tools/editor_tools/scripts/`, `tools/editor_tools/filesystem/` (~85 文件) | ~1000 行改动 |
| **SA-5B** | T2 (后半) | `tools/editor_tools/workspace/`, `tools/editor_tools/shader/`, `tools/editor_tools/animation/`, `tools/node_properties/`, `tools/runtime_tools/`, `tools/fallback/`, `tools/doc_tools/` (~86 文件) | ~1000 行改动 |
| **SA-5C** | M9,M10 | `tools/**/resource/*.hpp` (6 文件), `tools/**/toggle_*.hpp` (3 文件) | ~150 行改动 |

### 文件域验证

```
SA-5A: tools/meta/*, tools/editor_tools/scene_tree/*, tools/editor_tools/scripts/*, tools/editor_tools/filesystem/*
SA-5B: tools/editor_tools/workspace/*, tools/editor_tools/shader/*, tools/editor_tools/animation/*, tools/node_properties/*, tools/runtime_tools/*, tools/fallback/*, tools/doc_tools/*
SA-5C: tools/**/resource/*, tools/**/toggle_*
```

> **T2 拆分策略**：171 个工具按目录对半拆分。SA-5A 处理 meta + editor_tools 前半，SA-5B 处理 editor_tools 后半 + 其余。二者文件域完全不重叠。

### GATE 5 验证

```bash
uv run python build.py
# 这是最大规模的改动批次，需要特别仔细验证
# 1. 全量编译
# 2. 运行所有 YAML 测试
# 3. 抽查 10 个工具的 schema 输出正确性
```

---

## Batch 6 — 剩余合并 + 小修复

> **目标**：完成剩余工具合并和小修复
> **并行度**：3 subagents
> **预估耗时**：15-20 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-6A** | M1,M2 | `debugger_step_out.hpp`, `get_info.hpp` | ~10 行 |
| **SA-6B** | M3 (遗漏项) | 检查 Batch 2 的 M3 是否完整 | ~3 行 |
| **SA-6C** | T3 | GDScript/C# 工具对合并 (10→5 模板文件) | ~400 行改动 |

### 文件域验证

```
SA-6A: tools/**/debugger_step_out.hpp, tools/**/get_info.hpp
SA-6B: 检查性，可能无改动
SA-6C: tools/**/scripts/read_gd_script.hpp, read_csharp_script.hpp, write_*, patch_*, validate_*, list_*
```

> **注意**：T3 在 Batch 6 而非更早，因为它涉及 10 个脚本工具文件的合并，需要仔细处理。且 Batch 5 的 T2 可能已经修改了这些文件的 schema 部分，T3 需要在 T2 之后进行以避免冲突。

### GATE 6 验证

```bash
uv run python build.py
pytest tests/test_orchestrator.py -v --asyncio-mode=auto
```

---

## Batch 7 — 最终清理

> **目标**：移除所有已识别的死代码和 P3 小项
> **并行度**：2 subagents
> **预估耗时**：10-15 分钟

### Subagent 分配

| Subagent | 节点 | 文件域 | 预估行数 |
|----------|------|--------|----------|
| **SA-7A** | M5 (遗漏),M11 | `tool_base.hpp`, 多文件 P3 清理 | ~30 行 |
| **SA-7B** | 最终审查 | 全项目扫描，清理遗漏项 | 不定 |

### GATE 7 — 最终验证

```bash
# 1. Debug 编译
uv run python build.py

# 2. Release 编译
uv run python build.py --release

# 3. Unity OFF 编译
# 临时 GODOTMCP_UNITY_BUILD=OFF 编译

# 4. 完整测试套件
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# 5. 代码统计
# 确认代码行数减少、无新增 warning
```

---

## 并行度时间线

```
时间轴 →

Batch 0  ████████████████████  (40 min, 5 agents)
GATE     █
Batch 1  ████████              (20 min, 3 agents)
GATE     █
Batch 2  ████████████████████  (40 min, 4 agents)
GATE     █
Batch 3  ██████████████████████████  (50 min, 5 agents)
GATE     █
Batch 4  ████████████████████  (40 min, 4 agents)
GATE     █
Batch 5  ██████████████████████████  (50 min, 3 agents)
GATE     █
Batch 6  ████████████████████  (40 min, 3 agents)
GATE     █
Batch 7  ██████                (15 min, 2 agents)
GATE     █

总计: ~4.5 小时（含验证关卡）
```

---

## Subagent Prompt 模板

每个 subagent 启动时使用以下 prompt 结构：

```
## 任务
[节点 ID]: [标题]

## WHY
[为什么需要这个改动]

## WHAT
[具体要做什么，逐条列出]

## WHERE
[精确的文件路径和行号]

## DONE
[验证命令和预期结果]

## DON'T
[不要触碰的文件/不要做的改动]

## Context
[相关代码约定、依赖信息、注意事项]

## 输入
[上游节点的输出，如果有]
```
