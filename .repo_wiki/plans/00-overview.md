# GodotMCP 架构演进 — 总览

> 本文档为开发规划的主索引。所有子文档位于 `.repo_wiki/plans/` 目录。

## 文档索引

| 文档 | 内容 | 方法论 |
|------|------|--------|
| [01-architecture.md](01-architecture.md) | 架构决策、技术约束、跨平台策略、C++17 现代化 | 技术调研 |
| [02-dag.md](02-dag.md) | DAG 依赖分析、并行执行批次、Subagent 调度方案 | WBS、CPM |
| [03-sprint-1.md](03-sprint-1.md) | Sprint 1 详细实施方案（Q1/Q2/Q3/L3） | WBS 词典 |
| [04-sprint-2.md](04-sprint-2.md) | Sprint 2 详细实施方案（M3'/L2/L1） | WBS 词典 |
| [05-sprint-3.md](05-sprint-3.md) | Sprint 3 详细实施方案（L4+L5 WebSocket 同端口） | WBS 词典 |
| [06-testing.md](06-testing.md) | 测试与质量保证策略 | LogFrame MOVI |
| [07-risk-register.md](07-risk-register.md) | 风险登记表、假设日志、储备计算 | 风险管理 |
| [08-impact-map.md](08-impact-map.md) | 影响地图、SMART 目标、铁三角定调 | Impact Mapping |
| [09-wbs-pert.md](09-wbs-pert.md) | WBS 词典、PERT 估算、LogFrame、RACI 矩阵 | PERT/CPM/LogFrame/RACI |

## 业务目标 (WHY)

> **成为 Godot 生态中架构最先进的 MCP 工具，实现 AI 驱动的游戏开发闭环。**

详见 [08-impact-map.md](08-impact-map.md) 的影响地图分析。

## 项目管理铁三角

```
         质量 (Quality) ← P0 最高优先级
            ╱╲
           ╱  ╲
          ╱ ★  ╲
         ╱      ╲
        ╱────────╲
   时间            成本
  (P1)            (P2)
```

| 优先级 | 维度 | 约束 | 决策规则 |
|--------|------|------|---------|
| **P0** | **质量** | 零数据损坏、100% 破坏性确认 | 质量与时间冲突时：保质量 |
| **P1** | **时间** | PERT 期望 12.84d，含储备 20.41d | 时间与成本冲突时：保时间 |
| **P2** | **成本** | 1 人全职 + ≤11 Subagent 并发 | 成本固定，通过并行优化 |

详见 [08-impact-map.md §3](08-impact-map.md#3-项目管理铁三角)。

## PERT 工期估算

| 指标 | 值 | 来源 |
|------|-----|------|
| PERT 期望工期 | **12.84d** | 三点估算 (O/M/P) |
| 95% 置信区间 | [10.80d, 14.88d] | TE ± 2σ |
| 应急储备 | **2d** | 2σ 缓冲 |
| 管理储备 | **5d** | 15% 管理缓冲 |
| **含储备总工期** | **20.41d** | |
| 关键路径 | H→I→K→测试 | CPM 分析 |

详见 [09-wbs-pert.md §2](09-wbs-pert.md#2-pert-三点估算)。

## 项目快照

| 维度 | 当前状态 | Sprint 3 后目标状态 |
|------|---------|-------------------|
| C++ 源码根 | `extensions/src/` | 不变 |
| 工具总数 | 153（四层渐进式） | 155+（+get_game_instances, +rollback_operation） |
| 桥接端口 | `:9601`（独立 TCP） | 消除（WebSocket 同端口 `:9600`） |
| 配置项 | `http_port`, `bridge_port`, `http_host` | `http_port`, `http_host`（bridge_port 消除） |
| 多实例支持 | 不支持 | 天然多实例（WebSocket accept 多客户端） |
| 破坏性操作防护 | 无 UI 确认 | 确认对话框 + 会话控制 |
| MCP 标准兼容 | tools/list 仅返回 8 元工具 | 可选全量返回 153 工具 |
| 发布渠道 | 手动构建 | AssetLib + GitHub Release CI |
| 桥接协议 | raw TCP + `\n` 定界 | WebSocket 帧（RFC 6455） |
| 桥接角色 | 编辑器 TCP 客户端 → 游戏 TCP 服务器 | 编辑器 WS 服务器 ← 游戏 WS 客户端 |

## SMART 目标

> **在 20.41 个工作日内（含储备），完成 GodotMCP 从 v0.2.2-dev1 到 v1.0.0 的架构演进，实现 MCP 标准兼容、AI 操作安全防护、WebSocket 同端口多实例桥接，以及 AssetLib 自动化发布，覆盖 Windows/Linux/macOS 三平台。**

| SMART | 验证 |
|-------|------|
| **S**pecific | 3 Sprint、11 任务节点、明确交付物清单 |
| **M**easurable | 155+ 工具、100% 破坏性确认率、WS roundtrip 测试通过 |
| **A**chievable | PERT 期望 12.84d，Godot 源码验证 API 可用性 |
| **R**elevant | 对齐安全/高效/闭环三大业务目标（Impact Map 验证） |
| **T**ime-bound | 20.41d（含储备），3 Sprint，明确里程碑 |

详见 [08-impact-map.md §2](08-impact-map.md#2-smart-目标)。

## Sprint 总览

```
Sprint 1 (3d TE)        Sprint 2 (4d TE)         Sprint 3 (9d TE)
┌──────────────┐        ┌──────────────┐        ┌────────────────────┐
│ Q1 渐进披露   │        │ M3' Console  │        │ H  WS-Codec        │
│ Q2 确认对话框 │───────→│    增强       │        │ I  WS-HttpServer   │
│ Q3 协议修复   │        │ L2 操作回滚   │        │ J  WS-GameClient   │
│ L3 端口回退   │        │ L1 AssetLib  │        │ K  多实例路由       │
└──────────────┘        └──────────────┘        └────────────────────┘
     3d                      4d                       9d
                  关键路径: H→I→K→测试 = 12.84d (TE)
```

## ROI 矩阵

| ID | 建议 | 可行性 | PERT TE | 破坏性 | 预期收益 | 决策 |
|----|------|--------|---------|--------|---------|------|
| Q1 | 渐进式工具披露开关 | 高 | 1.08d | 低 | MCP 标准兼容 | **Sprint 1** |
| Q2 | 破坏性操作确认 UI | 高 | 3.17d | 低 | AI 安全防护 | **Sprint 1** |
| Q3 | 桥接协议 \n→长度前缀 | 高 | 2.17d | 中 | 消除消息错位 | **Sprint 1** |
| L3 | 多编辑器端口回退 | 高 | 1.58d | 低 | 多实例不冲突 | **Sprint 1** |
| M3' | McpConsole 增强 | 高 | 2.08d | 低 | 搜索 + Revert | **Sprint 2** |
| L2 | 操作回滚 MCP 工具 | 高 | 3.17d | 低 | LLM 自修复 | **Sprint 2** |
| L1 | AssetLib 发布流水线 | 高 | 3.17d | 低 | 用户获取 | **Sprint 2** |
| M1 | 工作线程池 | 中 | — | 高 | 主线程减负 | **暂缓** |
| M2 | HTTP 库迁移 | 低 | — | 极高 | ROI 不足 | **驳回** |
| H | WS 帧编解码 | 高 | 3.17d | 中 | WebSocket 基础 | **Sprint 3** |
| I | HttpServer WS | 高 | 4.33d | 中 | 同端口桥接 | **Sprint 3** |
| J | GameClient 重写 | 高 | 3.17d | 中 | 角色反转 | **Sprint 3** |
| K | 多实例路由 | 高 | 2.17d | 低 | 多实例支持 | **Sprint 3** |

## RACI 摘要

| 角色 | 职责 |
|------|------|
| **主 Agent** | **A** (Accountable) — 所有工作包的最终决策者、质量门禁、冲突仲裁 |
| **Subagent SA-1~SA-11** | **R** (Responsible) — 各自文件域内的执行者 |
| **用户** | **C** (Consulted) + **I** (Informed) — 关键决策确认、验收 |

完整 RACI 矩阵见 [09-wbs-pert.md §4](09-wbs-pert.md#4-raci-矩阵)。

## 风险与储备

| 类型 | 大小 | 用途 |
|------|------|------|
| 应急储备 | 2d | 已识别风险 (R1-R10) |
| 管理储备 | 5d | 未识别风险、范围变更 |
| **总储备** | **7d** | |

详见 [07-risk-register.md](07-risk-register.md)。
