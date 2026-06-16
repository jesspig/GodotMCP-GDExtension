# 验证策略 — 门禁与测试

## 验证层次

```
L1: 编译验证（每个 GATE 必须）
L2: 单元测试（YAML 测试套件）
L3: 功能验证（手动 / Playwright）
L4: 性能基准（可选）
```

---

## GATE 定义

### GATE 0 — 基础验证

> Batch 0 完成后（P0 bug 修复 + 构建系统 + 基础设施创建）

```bash
# L1: Debug 编译（Unity ON）
uv run python build.py
# 预期：编译成功，0 errors

# L1: Debug 编译（Unity OFF）
# 修改 ext/CMakeLists.txt: GODOTMCP_UNITY_BUILD=OFF
uv run python build.py --clean
# 预期：编译成功（验证 include 完整性）
# 完成后恢复 GODOTMCP_UNITY_BUILD=ON

# L2: YAML 测试（需 Godot 编辑器运行）
pytest tests/test_orchestrator.py -v --asyncio-mode=auto
# 预期：所有现有测试通过

# 手动验证：
# - X1: 打开 Godot 编辑器，触发 >100 条日志，检查 McpConsole 详情面板
# - X4: 用错误 token 调用 /mcp，确认返回 401
```

**通过标准**: 编译 0 errors + 现有测试 100% 通过

---

### GATE 1 — 构建完善验证

> Batch 1 完成后（警告级别提升 + args_get 合并 + 测试修复）

```bash
# L1: 编译（检查新 warning）
uv run python build.py
# 预期：编译成功，无新增 warning（或仅有预期的 C4244/C4267）

# L2: YAML 测试
pytest tests/test_orchestrator.py -v --asyncio-mode=auto
# 预期：所有测试通过

# 手动验证：
# - T6: 确认使用 args_get 和 args_get_typed 的工具正常工作
```

**通过标准**: 编译 0 errors + 无新增 warning + 测试通过

---

### GATE 2 — 安全迁移验证

> Batch 2 完成后（MemdeleteGuard 应用 + 死代码清理 + logger 修复）

```bash
# L1: 编译
uv run python build.py

# L2: YAML 测试
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# 手动验证：
# - X5: 创建节点后触发失败路径，确认无崩溃
# - U1: 检查日志轮转是否正常工作（生成 >10MB 日志）
```

**通过标准**: 编译通过 + 测试通过 + 无内存泄漏（手动验证）

---

### GATE 3 — 架构重构验证

> Batch 3 完成后（HandlerRegistry 去重 + 死代码清理 + 性能优化第一批）

```bash
# L1: 编译
uv run python build.py

# L2: 完整 YAML 测试
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# L3: 功能验证
# - A1: 调用 tools/list，确认返回所有工具（数量 = 171 - 9 已删除 = 162）
# - A1: 调用 tools/list with category filter，确认分类正确
# - A2: 确认 pending restart 逻辑正确（或删除后不影响功能）
# - A3: 打开大型场景（1000+ 节点），调用 get_scene_tree，确认响应 < 1MB
# - M4: 修改 shader uniform，按 Ctrl+Z，确认撤销成功
```

**通过标准**: 编译通过 + 测试通过 + 功能验证全部通过

---

### GATE 4 — 性能优化验证

> Batch 4 完成后（搜索索引 + 工具合并）

```bash
# L1: 编译
uv run python build.py

# L2: YAML 测试
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# L4: 性能基准（可选）
# - P1: 调用 search_tools 100 次，记录平均耗时
#   目标：< 5ms（当前可能 50-100ms）
# - P7: 调用 get_performance_monitors，确认返回所有 59 个 monitor
```

**通过标准**: 编译通过 + 测试通过 + 搜索性能 < 10ms

---

### GATE 5 — Schema 迁移验证（最关键）

> Batch 5 完成后（171 个工具的 schema 替换）

```bash
# L1: 编译
uv run python build.py

# L2: 完整 YAML 测试
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# L3: Schema 一致性验证
# 编写脚本对比替换前后的 schema 输出：
# 1. 在替换前保存所有工具的 schema 到 baseline.json
# 2. 替换后重新获取所有工具的 schema
# 3. diff baseline.json 和 new.json，确认语义等价
```

**Schema 对比脚本**（伪代码）：
```python
# 对比逻辑：
# - type 字段必须完全一致
# - properties 的 key 集合必须一致
# - 每个 property 的 type/description/default 必须一致
# - required 数组必须一致（顺序可不同）
```

**通过标准**: 编译通过 + 测试通过 + schema diff 为空（或仅有格式差异）

---

### GATE 6 — 工具合并验证

> Batch 6 完成后（GDScript/C# 合并 + 小修复）

```bash
# L1: 编译
uv run python build.py

# L2: YAML 测试
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# L3: 功能验证
# - T3: 调用 read_gd_script 和 read_csharp_script，确认功能不变
# - M1: 调用 debugger_step_out，确认行为说明正确
```

**通过标准**: 编译通过 + 测试通过 + 功能验证通过

---

### GATE 7 — 最终验证

> 所有批次完成后

```bash
# L1: Debug 编译
uv run python build.py

# L1: Release 编译
uv run python build.py --release

# L1: Unity OFF 编译
# GODOTMCP_UNITY_BUILD=OFF

# L2: 完整测试套件
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# L3: 代码统计
# - 确认工具总数 = 162 - 10（合并删除） = ~152
# - 确认代码行数减少 > 2000 行
# - 确认无新增 warning

# L4: 性能对比（可选）
# - search_tools 性能对比
# - HTTP 响应构建性能对比
```

**通过标准**: 3 种编译模式全部通过 + 测试通过 + 代码行数减少

---

## 回归测试清单

每个 GATE 都需要验证以下核心功能不退化：

| 功能 | 验证方法 | 相关 GATE |
|------|----------|-----------|
| 工具列表 | `tools/list` 返回正确数量和分类 | 0,3,5,6 |
| 工具搜索 | `tools/list?query=xxx` 返回相关工具 | 3,4 |
| 工具详情 | `tools/list?name=xxx` 返回完整 schema | 5 |
| 场景树操作 | add/remove/move node | 2,3 |
| 脚本操作 | read/write/patch GDScript + C# | 2,6 |
| 文件系统 | create/delete/move file | 2 |
| 运行时桥接 | get/set property at runtime | 0,4 |
| HTTP 认证 | 正确/错误 token 的响应 | 0 |
| 日志系统 | 日志写入 + 轮转 | 2 |
| Undo/Redo | Ctrl+Z 撤销场景修改 | 3 |
| Shader uniform | 设置 + 撤销 | 3 |

---

## 快速回滚命令

如果某个 GATE 失败，回滚到上一个成功状态：

```bash
# 查看 git log 找到上一个 GATE 通过的 commit
git log --oneline -10

# 回滚到该 commit
git reset --hard <commit-hash>

# 重新编译验证
uv run python build.py
```

**建议**：每个 GATE 通过后立即 commit，commit message 格式：
```
chore: GATE N passed — [批次描述]
```
