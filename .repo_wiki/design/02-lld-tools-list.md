# LLD：渐进式披露优化 & `tools/list_changed` SSE 通知

> **状态:** 草案 · **版本:** 2.0 · **日期:** 2026-06-20  
> **受影响组件:** `HandlerRegistry`、`McpHandler`、`McpToolRegistry`  
> **相关 ADR:** [00-architecture.md](00-architecture.md)

---

## 1. 设计原则

### 1.1 核心立场

渐进式披露是 GodotMCP 的**核心优势**，不是缺陷。

| 维度 | 全量返回方案 | 渐进式披露方案（当前+优化） |
|------|-------------|--------------------------|
| tools/list 返回 | 153 个工具 schema（~50KB JSON） | 7 个元工具（~3KB JSON） |
| AI 首次 token 消耗 | 高（全部 schema 进入 context） | 低（仅元工具进入 context） |
| 工具发现路径 | 单次调用即可 | 元工具链 + 搜索引擎 |
| 标准 MCP 兼容性 | 完全兼容 | 需 AI 使用元工具发现 |
| token 效率 | 每次对话刷新全部 schema | 按需查询，按需加载 |

**结论**：AI 助手（Claude Code、Codex、Cline）足够智能，能够调用元工具来发现所需功能。保留 `tools/list` 仅返回元工具的行为，将优化精力集中在**让发现链更快、更精确、更省 token**。

### 1.3 来自生产环境的验证

实际 MCP 客户端的已知问题进一步支持渐进式披露设计：

| 客户端 | 已知 Issue | 对全量方案的影響 | 对渐进式披露的影响 |
|--------|-----------|-----------------|-------------------|
| Claude Code | `#45` — `tools/list` 缓存 1 小时 TTL | 153 工具列表静默过期 | **无影响** — 元工具始终即时可用 |
| Claude Code | `#24785` — 分页 broken，`nextCursor` 被忽略 | 超出一页的工具静默丢失 | **无影响** — 发现链不走分页 |
| Claude Code | `#4118` — `list_changed` 通知不生效 | 工具变更后客户端看不到更新 | **无影响** — AI 每次 `tools/call` 都是实时查询 |
| Claude Web | `#45` comment — `list_changed` 后新工具查不到 | 新工具无法调用 | **无影响** — `find_tool` / `get_tools` 总是返回最新 |

MCP 社区也正在向渐进式披露方向演进：`Discussion #1923` 提案 `tools/list_summary` + `tools/get` 模式（预计节省 88% token），`SEP-1576` 提案基于 embedding 的工具检索。GodotMCP 的元工具链是这个方向的**生产级先行实现**。

### 1.2 优化方向

1. **元工具精简** — 合并/压缩元工具数量，减少首次交互开销
2. **缓存加速** — 分类树、搜索索引缓存，避免重复计算
3. **搜索引擎增强** — 更快的模糊匹配，更好的排序
4. **SSE 通知补完** — `tools/list_changed` 在 SDK 工具注册/注销时触发

---

## 2. 元工具精简

### 2.1 现状

当前 7 个元工具（`is_meta()==true`）：

```json
get_info              // 连接状态、引擎版本、项目配置等
get_tools             // 按分类路径列出工具（不含子分类）
get_categories        // 分类树，支持 path 钻取和 max_depth
get_tool_detail       // 单工具完整 schema（已合并入 get_tools 的 detail=true 模式）
find_tool             // 4 阶段权重搜索引擎
call_tool             // 兜底调用任意工具
list_settings         // 列出项目设置（is_meta=false，属 editor_tools/settings）
```

另有客户端配置生成功能位于底部面板 `McpDock` UI（`client_config_registry.hpp` 实现），作为 `get_info` 的可选返回字段合并计划（见下文）。

### 2.2 精简策略

**合并 `get_tools` + `get_tool_detail` → `get_tools`（增强）**

`get_tools` 简化为两个模式：按名查详情 / 无参列出全部。分类浏览由 `get_categories` 负责，搜索由 `find_tool` 负责。

```json
// get_tools(name="add_node")
// →
{
  "id": "add_node",
  "name": "Add a new node as child",
  "description": "...",
  "category_path": "editor_tools/scene_tree",
  "parameters": [...],
  "required": [...],
  "usage_example": "..."
}

// get_tools()
// →
{
  "tools": [
    {"id": "add_node", "name": "Add a new node", "description": "...", "category": "editor_tools/scene_tree"},
    ...
  ],
  "count": 153
}
```

这样 `get_tool_detail` 可以作为独立工具移除，节省 1 个元工具槽位。`category` 和 `detail` 参数不再需要——职责分离到 `get_categories` 和 `find_tool`。

**将客户端配置纳入 `get_info`（取代独立工具/UI）**

当前客户端配置生成位于底部面板 `McpDock` UI（`client_config_registry.hpp`）。将其纳入 `get_info` 的可选返回字段，用户和 AI 均可通过 `tools/call` 直接获取：

```json
// get_info(include_configs=true)
{
  "connection": {"status": "ok"},
  "engine": {"version": "4.6.stable"},
  "plugin": {"builtin_tools": 153},
  "client_configs": {
    "claude_code": {"url": "http://localhost:9600/mcp", ...},
    "cursor": {"url": "http://localhost:9600/mcp", ...}
  }
}
```

这样节省 1 个元工具槽位（当前为 UI 功能，可转为可选返回字段）。

### 2.3 精简后元工具清单（目标：7→5 个，计客户端配置合并）

| 工具 | 功能 | schema 大小 |
|------|------|:----------:|
| `get_info` | 精简版：引擎/项目/桥接状态 + 可选客户端配置 | ~1KB |
| `get_tools` | 按名查工具详情 / 无参列出全部工具（分类浏览交给 get_categories） | ~0.4KB |
| `get_categories` | 分类树，max_depth 控制钻取深度 | ~0.5KB |
| `find_tool` | 4 阶段权重搜索引擎（不变） | ~0.5KB |
| `call_tool` | 兜底调用 | ~0.3KB |

**精简效果**：7 → 5（合并 get_tools+get_tool_detail，客户端配置转入 get_info 可选字段，list_settings 移至 editor_tools/settings），元工具首次交互 overhead 减少 ~28%。

---

## 3. 缓存优化

### 3.1 分类树缓存

**问题**：`get_categories()` 每次调用重新遍历 `itool_table_` 构建树（`handler_registry.cpp:205`），O(n) 开销。

**优化**：树构建后缓存，仅当工具注册/注销时标记 `categories_dirty_`：

```cpp
// handler_registry.cpp - 已有机制，增强
mutable Array categories_cache_;
mutable bool categories_dirty_ = true;

// 在 register_tool() 和 unregister_custom_tool() 中：
categories_dirty_ = true;

// 在 get_categories() 中：
if (categories_dirty_ || rebuild_needed) {
    categories_cache_ = build_category_tree();
    categories_dirty_ = false;
}
return categories_cache_;
```

### 3.2 搜索索引持久化

**问题**：`find_tool` 每次搜索实时 tokenize 所有工具的描述文本。

**优化**：注册时增量构建 `search_index_`（已有 `PackedStringArray tokens` 字段），搜索时只做字符串匹配不重新 tokenize：

```cpp
// handler_registry.cpp:register_tool()
SearchIndexEntry entry;
entry.tokens = tokenize(tool->name() + " " + tool->brief() + " " + tool->description());
search_index_[name] = entry;
```

### 3.3 schema 缓存

**问题**：`input_schema()` 每次调用构建 `Dictionary`。

**优化**：`ITool` 基类已有 `mutable std::optional<Dictionary> schema_cache_`（`tool_base.hpp:102`），确保缓存生效。审计工具实现，确保 `build_input_schema()` 不重复分配。

---

## 4. `tools/list_changed` SSE 通知补完

### 4.1 现状

- `McpHandler::notify_tools_list_changed()` 已实现（`mcp_handler.cpp:581-585`）
- `McpToolRegistry` 注册/注销自定义工具时已触发（`mcp_tool_registry.cpp:285-288`）
- **但**：注册/注销的变换未通知 `McpHandler` → SSE 不推送

### 4.2 修复

```cpp
// McpToolRegistry 中注册/注销工具后：
void McpToolRegistry::_on_tools_changed() {
    if (mcp_handler_) {
        mcp_handler_->notify_tools_list_changed();
    }
}
```

这是少代码、高价值的修复——SDK 工具开发者第一次能实时感知工具变更。

---

## 5. 搜索引擎增强

### 5.1 关键词补全

`find_tool` 增加 `autocomplete` 模式，支持前缀模糊匹配（已有 `get_search_suggestions`，`handler_registry.cpp:468`）：

```json
// find_tool(query="anim", autocomplete=true)
// →
["animation/create_animation_player", "animation/create_animation_clip", 
 "animation/create_animation_tree", "animation/add_animation_track"]
```

### 5.2 频率索引优化

当前频率索引 `freq_index_` 在每次工具调用时更新（`handler_registry.cpp:471`），但从未持久化。考虑改为 LRU 队列，避免无限增长：

```cpp
// 频率衰减：每次查询时给所有频率 * 0.99，而非累加
// 避免冷门工具因历史调用多而持续排名靠前
```

---

## 6. 向后兼容

- `tools/list` 行为**完全不变**——仍返回元工具（当前 7 个，精简后 5 个）
- 现有 MCP 客户端无需任何配置变更
- 现有 AI 工作流（`get_categories` → `get_tools` → `get_tool_detail`）完全不变
- `notifications/tools/list_changed` 是新增（修复）功能，不会破坏现有客户端

---

## 7. 测试计划

| ID | 名称 | 预期 |
|:--:|------|------|
| UT1 | `tools/list` 返回元工具 | 全部 `is_meta==true`（当前 7 个，精简后 5 个） |
| UT2 | 渐进式发现链完整 | `get_categories` → `get_tools(...)` → 工具存在 |
| UT3 | 搜索引擎匹配 | `find_tool("add_node")` → `[name: "add_node", weight: 1000]` |
| UT4 | 分类树缓存 | 二次调用 `get_categories` 快于首次 |
| UT5 | schema 缓存 | 二次调用 `input_schema()` 快于首次 |
| IT1 | SDK 工具注册触发 SSE | 订阅 SSE → 注册自定义工具 → 收到 `tools/list_changed` |
| IT2 | SDK 工具注销触发 SSE | 同上，注销时也触发 |
| IT3 | 搜索补全 | `find_tool(query="anim", autocomplete=true)` 返回动画相关工具名 |
| IT4 | 元工具精简后 `tools/list` 返回 5 个 | 精简后验证 |

---

## 8. 验收标准

1. `tools/list` 返回元工具（当前 7 个，精简后 5 个），行为不变
2. `get_categories` 响应时间 < 1ms（首次） / < 0.1ms（缓存命中）
3. `find_tool` 模糊搜索在 153 个工具中 < 2ms
4. SDK 工具注册后 SSE `tools/list_changed` 通知在 1 帧内送达
5. Schema 缓存生效：同工具二次调用 `input_schema()` 零分配
6. 所有 153 个工具仍可通过渐进式发现链完整访问
7. 全部现有 YAML 测试通过

---

## 9. 改动量估算

| 文件 | 改动 | 行数 |
|------|------|:----:|
| `meta/get_tools.hpp` | 简化：移除 category/detail 参数，name→详情 / 无参→列表 | ~-30 (净减) |
| `meta/get_info.hpp` | 增强：可选返回客户端配置 | ~15 |
| `meta/get_tool_detail.hpp` | 移除（功能并入 get_tools） | ~0 (删除) |
| `generate_client_config`（UI 功能，`client_config_registry.hpp`） | 移除独立 UI，改为 get_info 可选返回字段 | ~0 (修改) |
| `register/register_meta.hpp` | 移除 2 行 | -2 |
| `register_itools.cpp` | 移除 2 个 `#include` | -2 |
| `handler_registry.cpp` | 缓存增强 + 搜索索引优化 | ~20 |
| `mcp_tool_registry.cpp` | 确保 SSE 通知触发 | ~3 |
| `editor_plugin.cpp` | 无变更 | 0 |
| **合计** | | **~+66 / -4** |
