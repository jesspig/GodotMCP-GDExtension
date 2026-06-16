# 风险评估与缓解方案

## 风险矩阵

| 风险 | 概率 | 影响 | 等级 | 缓解方案 |
|------|------|------|------|----------|
| Unity Build 编译失败（include 缺失暴露） | 高 | 中 | **高** | Batch 0 加 B10 SKIP_UNITY_BUILD；每批次 Unity ON+OFF 双模式验证 |
| Schema 替换引入语义变化 | 中 | 高 | **高** | GATE 5 schema diff 对比脚本；逐目录替换+编译验证 |
| MemdeleteGuard 误用于 Godot Wrapped 子类 | 中 | 高 | **高** | I1 模板加 `static_assert` 检查；代码审查 |
| add_child 所有权修复遗漏 | 中 | 高 | **高** | 全局搜索 `add_child.*memdelete` 模式；X5+T1 合并处理 |
| read_response 异步化改动范围过大 | 中 | 中 | **中** | 先减小 timeout 到 100ms 作为临时方案；异步化作为后续迭代 |
| rapidyaml URL 拉取失败（tarball 不含 c4core） | 低 | 高 | **中** | 保留 GIT 方式作为 fallback；测试 tarball 内容后再改 |
| 警告级别提升后大量新 warning | 高 | 低 | **中** | B8 延后到 B2 之后；先 per-target 抑制，逐步修复 |
| Subagent 并行修改同一文件 | 低 | 高 | **中** | 文件域矩阵严格检查；批次内 subagent 文件域不重叠 |
| 工具合并导致 MCP 客户端不兼容 | 中 | 中 | **中** | 保留工具 name 不变；只删除冗余工具，不合并功能不同的工具 |
| Godot 编辑器 DLL 文件锁导致编译失败 | 高 | 低 | **低** | 编译前关闭 Godot 编辑器；`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded` 已减少 PDB 锁 |

---

## 高风险任务详细缓解方案

### 风险 1: Unity Build 编译失败

**场景**: Batch 5 大规模修改 171 个工具文件后，Unity Build 合并翻译单元时出现符号冲突或缺失 include。

**缓解**:
1. B10 已将 `register_itools.cpp` 排除出 Unity Build
2. 每个 subagent 完成其文件域后立即编译验证
3. 如果 Unity ON 失败但 OFF 成功，说明是 Unity 特有问题：
   - 检查是否有匿名命名空间中的同名符号
   - 检查是否有文件作用域 `static` 变量冲突
   - 对问题文件添加 `SKIP_UNITY_BUILD ON`
4. CI 应同时测试 Unity ON 和 OFF

**回滚**: 如果无法快速修复，恢复 Unity ON（当前状态），将 SKIP_UNITY_BUILD 扩展到更多文件。

---

### 风险 2: Schema 替换引入语义变化

**场景**: `SchemaBuilder` 生成的 Dictionary 与手动构建的 Dictionary 在 key 顺序、类型表示等方面有细微差异，导致 MCP 客户端解析失败。

**缓解**:
1. GATE 5 的 schema diff 脚本必须对比语义而非字符串
2. 对比维度：
   - `type` 字段值完全一致
   - `properties` 的 key 集合完全一致
   - 每个 property 的 `type`、`description`、`default`、`enum` 完全一致
   - `required` 数组元素完全一致（顺序可不同）
3. 逐目录替换，每替换一个目录就编译+测试
4. 保留手动 schema 代码作为参考（git history 中可查）

**回滚**: `git checkout HEAD~1 -- tools/` 恢复所有工具文件。

---

### 风险 3: MemdeleteGuard 误用

**场景**: 将 `MemdeleteGuard<Node>` 用于已被 `add_child` 接管的节点，析构时 `memdelete` 导致 double-free。

**缓解**:
1. 模板添加文档注释：
```cpp
/// WARNING: Only use for objects NOT yet added to scene tree.
/// Once add_child() succeeds, ownership transfers to the tree.
/// Call dismiss() after successful add_child().
```
2. X5 先修复所有权问题（明确哪些路径需要 `dismiss()`），T1 再用 guard 替换
3. 代码审查重点检查 `dismiss()` 调用位置

**回滚**: 恢复手动 `memdelete` 回滚链（git revert）。

---

### 风险 4: read_response 异步化

**场景**: 将阻塞式 `read_response()` 改为非阻塞需要修改调用方，改动范围可能超出预期。

**缓解**:
1. **分两步走**：
   - Step 1（X2）：将 timeout 从 2000ms 减小到 100ms，缓解 UI 冻结
   - Step 2（后续迭代）：完整异步化
2. 如果 Step 1 导致超时错误增多，考虑：
   - 增加重试逻辑
   - 或改为 500ms timeout + 非阻塞检查

**回滚**: 恢复 2000ms timeout。

---

## Subagent 失败处理

### 单个 Subagent 失败

如果批次中某个 subagent 失败（编译错误、逻辑错误）：

1. **不影响同批次其他 subagent**（文件域隔离保证）
2. 修复失败的 subagent，重新运行
3. 如果修复需要 > 15 分钟，跳过该节点，标记为 "deferred"
4. 批次 GATE 验证时，跳过的节点不阻塞其他节点

### 整个批次失败

如果批次 GATE 验证失败：

1. 识别失败原因（编译错误 vs 测试失败 vs 功能退化）
2. 如果是编译错误：定位到具体 subagent 的改动，revert 该 subagent 的改动
3. 如果是测试失败：分析测试输出，定位到具体节点
4. 如果是功能退化：回滚整个批次，重新设计

### 级联失败

如果 Batch N 的失败导致 Batch N+1 无法开始：

1. 评估是否可以跳过失败节点，让 Batch N+1 的部分节点继续
2. 如果不可跳过，将失败节点的修复作为 Batch N 的补充任务
3. 更新 DAG 依赖图，记录实际执行路径

---

## 降级策略

某些优化如果实现难度超出预期，可以降级为更简单的方案：

| 原始方案 | 降级方案 | 触发条件 |
|----------|----------|----------|
| T2: 171 工具全部用 SchemaBuilder | 只替换 50 个最复杂的工具 | 替换过程中发现 > 10 个语义差异 |
| T3: GDScript/C# 模板合并 | 只提取公共函数，不合并文件 | 模板特化过于复杂 |
| A1: ToolInfo 完全去重 | ToolInfo 保留但添加同步断言 | 去重后搜索/分类功能异常 |
| X2: read_response 完全异步化 | 仅减小 timeout 到 100ms | 异步化改动范围过大 |
| P1: 预构建搜索索引 | 缓存最近一次搜索结果 | 索引维护逻辑过于复杂 |
| B4: rapidyaml URL+SHA256 | 保持 GIT 但添加 GIT_SUBMODULES 限定 | tarball 不含 c4core |

---

## 执行纪律

1. **每个 GATE 通过后立即 commit** — 提供回滚锚点
2. **不要跳过 GATE** — 即使"只是小改动"
3. **不要合并批次** — 即使"前一批看起来很简单"
4. **记录实际耗时** — 用于未来项目估算
5. **记录意外发现** — 执行过程中发现的新问题记入 backlog
