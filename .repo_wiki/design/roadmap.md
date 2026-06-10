# 功能优化路线图

> 竞品分析与功能缺口追踪。详细背景见 [ADR-014](decisions.md#ADR-014)。

## 竞品生态全景

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

## 功能缺口

### P0 — 必须补（竞品都有，缺失即硬伤）

| 能力 | 本项目 | Meow | alexmeckes | Sods2 | satellite |
|------|--------|------|------------|-------|-----------|
| 游戏运行时桥接 | ❌ | ✅ | ✅ | ✅ | ✅ |
| 编辑器/游戏截图 | ❌ | ✅ | ✅ | ✅ | ✅ |
| 脚本读写编辑 | ❌ | ✅ | ✅ | ✅ | ❌ |

### P1 — 应该补（显著提升竞争力）

| 能力 | 本项目 | Meow | alexmeckes | Sods2 | satellite |
|------|--------|------|------------|-------|-----------|
| 动画系统工具 | ❌ | ✅ | ✅ | ❌ | ✅ |
| UI/Control 工具 | ❌ | ✅ | ✅ | ❌ | ❌ |
| TileMap 操作 | ❌ | ✅ | ❌ | ❌ | ✅ |
| 碰撞形状一键创建 | ❌ | ✅ | ❌ | ❌ | ❌ |

### P2 — 加分项（差异化竞争）

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

## 核心竞争力

本项目**唯一不可替代的优势**：**C++ GDExtension 进程内运行 + 零外部依赖**。

- 启动速度：零（插件随编辑器加载）
- 调用延迟：函数调用级别，非 IPC
- 资源开销：无额外进程
- 部署复杂度：一个 `.dll` 文件

---

## Phase 1 (P0) — 入场券

> 消除所有"竞品都有，缺失即硬伤"的功能缺口。

### 1.1 游戏运行时桥接

**实现路径**：通过 `EditorDebuggerPlugin` 与运行中的游戏双向通信。

| 子能力 | 实现方式 | 参考 |
|--------|---------|------|
| 输入注入（键盘/鼠标/Action） | EditorDebuggerPlugin → 运行中游戏 | Meow、alexmeckes |
| 运行时属性读取 | 游戏端 GDScript autoload 暴露属性查询 API | satelliteoflove |
| 运行时场景树获取 | 游戏端遍历场景树序列化后返回 | Meow |
| GDScript 表达式执行 | 游戏端 `Expression.execute()` | Sods2、Dreamer568 |

**架构决策**：
- 在 GDExtension 内启动一个 TCP/WebSocket 服务端（端口待定，建议 9601）
- 游戏端通过 `EditorDebuggerPlugin` 注入一个 autoload 脚本，该脚本连接到此端口
- MCP 工具调用时，GDExtension 通过此连接转发请求到运行中的游戏
- 游戏退出时连接自动断开，GDExtension 检测到断开后返回"游戏未运行"错误

**注意**：`EditorDebuggerPlugin` 是编辑器内部类，不在 godot-cpp 绑定中。需通过 `find_children("*", "EditorDebuggerPlugin", true, false)` 遍历场景树 + `call()` 动态调用。

**追踪**：
- [ ] 实现 EditorDebuggerPlugin 连接检测
- [ ] 实现输入注入（键盘/鼠标/Action）
- [ ] 实现运行时属性读取
- [ ] 实现运行时场景树获取
- [ ] 实现 GDScript 表达式执行
- [ ] 编写 YAML 测试用例

### 1.2 编辑器/游戏截图

**实现路径**：
- 编辑器截图：`EditorInterface::get_singleton()->get_editor_viewport_2d/3d()->get_texture()->get_image()`
- 游戏截图：通过运行时桥接获取游戏视口截图
- 返回格式：MCP `ImageContent`（本项目已支持 spec 2025-03-26）

**追踪**：
- [ ] 实现编辑器 2D/3D 视口截图
- [ ] 实现游戏运行时截图（依赖 1.1 桥接）
- [ ] 返回 MCP ImageContent 格式
- [ ] 编写 YAML 测试用例

### 1.3 脚本读写编辑

| 工具 | 实现方式 |
|------|---------|
| `read_script` | `FileAccess::open()` 读取 `.gd` 文件 |
| `write_script` | `FileAccess::open()` 写入新文件 |
| `edit_script` | 读取 → 修改 → 写入 |
| `patch_script` | 精确替换指定行/段（参考 Dreamer568 的 `patch_script`） |
| `attach_script` | `node.set_script(ResourceLoader::load(path))` |
| `detach_script` | `node.set_script(Variant())` |

**追踪**：
- [ ] 实现 `read_script`
- [ ] 实现 `write_script`
- [ ] 实现 `edit_script`
- [ ] 实现 `patch_script`
- [ ] 实现 `attach_script`
- [ ] 实现 `detach_script`
- [ ] 编写 YAML 测试用例

---

## Phase 2 (P1) — 竞争力

> 补齐显著提升竞争力的功能模块。

### 2.1 动画系统工具

| 工具 | 实现方式 |
|------|---------|
| `create_animation_player` | 创建 AnimationPlayer 节点 + AnimationLibrary |
| `create_animation_clip` | 在 AnimationLibrary 中创建 Animation |
| `add_animation_track` | 添加 value/position/rotation/scale 轨道 |
| `set_keyframe` | 插入/修改/删除关键帧 |
| `get_animation_info` | 查询动画列表、轨道结构、关键帧数据 |

**追踪**：
- [ ] 实现 `create_animation_player`
- [ ] 实现 `create_animation_clip`
- [ ] 实现 `add_animation_track`
- [ ] 实现 `set_keyframe`
- [ ] 实现 `get_animation_info`
- [ ] 编写 YAML 测试用例

### 2.2 UI/Control 工具

| 工具 | 实现方式 |
|------|---------|
| `create_control` | 创建 Control 子类节点（Button/Label/Panel 等） |
| `set_layout_preset` | 设置锚点预设（full_rect/center 等） |
| `set_theme_override` | 批量设置主题覆盖（颜色/字体/字号） |
| `create_stylebox` | 创建 StyleBoxFlat 并应用到节点 |

**追踪**：
- [ ] 实现 `create_control`
- [ ] 实现 `set_layout_preset`
- [ ] 实现 `set_theme_override`
- [ ] 实现 `create_stylebox`
- [ ] 编写 YAML 测试用例

### 2.3 TileMap 操作

| 工具 | 实现方式 |
|------|---------|
| `set_tilemap_cells` | 批量放置瓦片到指定网格坐标 |
| `erase_tilemap_cells` | 批量擦除指定坐标的瓦片 |
| `get_tilemap_info` | 查询 TileMapLayer 元信息 |

**追踪**：
- [ ] 实现 `set_tilemap_cells`
- [ ] 实现 `erase_tilemap_cells`
- [ ] 实现 `get_tilemap_info`
- [ ] 编写 YAML 测试用例

### 2.4 碰撞形状一键创建

| 工具 | 实现方式 |
|------|---------|
| `create_collision_shape` | 一步创建 CollisionShape2D/3D + 配置形状（RectangleShape2D/CircleShape2D/CapsuleShape2D 等 9 种） |

**追踪**：
- [ ] 实现 `create_collision_shape`
- [ ] 编写 YAML 测试用例

---

## Phase 3 (P2) — 差异化

> 实现竞品没有或只有少数竞品有的差异化功能。

### 3.1 断点调试集成

参考 Sods2 的实现，通过 `EditorDebuggerPlugin` 与调试器通信：

| 工具 | 实现方式 |
|------|---------|
| `set_breakpoint` | 通过 `EditorDebuggerNode` 设置断点 |
| `remove_breakpoint` | 移除断点 |
| `list_breakpoints` | 列出所有活跃断点 |
| `get_stack_trace` | 获取当前堆栈跟踪 |
| `get_locals` | 获取局部变量 |
| `step_over/into/out` | 单步执行 |
| `continue` | 继续执行 |

**注意**：`EditorDebuggerNode` 是编辑器内部类，需通过 `find_children()` + `call()` 动态调用。

**追踪**：
- [ ] 实现 `set_breakpoint` / `remove_breakpoint` / `list_breakpoints`
- [ ] 实现 `get_stack_trace` / `get_locals`
- [ ] 实现 `step_over` / `step_into` / `step_out` / `continue`
- [ ] 编写 YAML 测试用例

### 3.2 MCP Resources + Prompts

| 资源 URI | 内容 |
|----------|------|
| `godot://scene/current` | 当前打开的场景信息 |
| `godot://project/info` | 项目元数据 |
| `godot://script/current` | 当前打开的脚本 |

| Prompt 模板 | 用途 |
|-------------|------|
| `create_player_controller` | 创建玩家控制器 |
| `setup_scene_structure` | 设置场景结构 |
| `debug_physics` | 调试物理问题 |
| `create_ui_layout` | 创建 UI 布局 |
| `setup_animation` | 设置动画 |

**追踪**：
- [ ] 实现 `godot://scene/current` Resource
- [ ] 实现 `godot://project/info` Resource
- [ ] 实现 `godot://script/current` Resource
- [ ] 实现 Prompt 模板（5+）
- [ ] 编写 YAML 测试用例

### 3.3 项目可视化器

参考 tomyud1 的方案：启动一个 HTTP 服务（如 `:6510`）提供浏览器端力导向图，展示脚本关系、场景图、依赖关系。

**追踪**：
- [ ] 实现脚本关系图数据收集
- [ ] 实现 HTTP 可视化服务
- [ ] 实现浏览器端力导向图渲染

### 3.4 Godot 文档查询

| 工具 | 实现方式 |
|------|---------|
| `get_class_info` | 调用 Godot 的 `ClassDB` API |
| `search_docs` | 使用 `--doctool` 生成的文档缓存 |
| `get_best_practices` | 预置最佳实践知识库 |

**追踪**：
- [ ] 实现 `get_class_info`
- [ ] 实现 `search_docs`
- [ ] 实现 `get_best_practices`
- [ ] 编写 YAML 测试用例

### 3.5 项目脚手架

| 工具 | 实现方式 |
|------|---------|
| `create_project` | 调用 Godot CLI 创建新项目 |

**追踪**：
- [ ] 实现 `create_project`
- [ ] 编写 YAML 测试用例

### 3.6 导出/插件/输入映射管理

| 工具 | 实现方式 |
|------|---------|
| `get_export_presets` / `run_export` | 读取/调用导出预设 |
| `list_plugins` / `set_plugin_enabled` | 管理编辑器插件 |
| `list_input_actions` / `add_input_action` | 管理输入映射 |

**追踪**：
- [ ] 实现导出工具
- [ ] 实现插件管理工具
- [ ] 实现输入映射工具
- [ ] 编写 YAML 测试用例

### 3.7 Shader 工具

| 工具 | 实现方式 |
|------|---------|
| `create_shader` | 创建 `.gdshader` 文件 |
| `read_shader` | 读取 shader 源码 |
| `apply_shader_preset` | 应用预设效果（dissolve/outline/hologram 等，参考 alexmeckes 的 11 种预设） |

**追踪**：
- [ ] 实现 `create_shader`
- [ ] 实现 `read_shader`
- [ ] 实现 `apply_shader_preset`
- [ ] 编写 YAML 测试用例

---

## 实施原则

1. **每阶段完成后才能进入下一阶段**：P0 未完成前不开始 P1，以此类推
2. **优先利用 GDExtension 优势**：做 Node.js 方案做不到的事（底层 API 调用、实时性能数据、编辑器 UI 自动化）
3. **保持 codegen 体系**：新工具应尽量利用 `// @tool register` + codegen 自动注册
4. **工具发现优化**：~11,734 工具中大部分是属性 get/set 噪音，考虑按使用频率分级或合并为通用工具
5. **每项功能完成后**：更新 `.repo_wiki/log.md` 并补充对应模块文档
6. **测试先行**：每项新工具必须附带 YAML 测试用例

---

## 架构优化阶段（Phase 4 — ADR-015）

> 待 P1（动画/UI/TileMap/碰撞）全部完成后启动。完整方案见 [ADR-015](decisions.md#ADR-015)。

### Phase 0 — 基础架构

| 任务 | 状态 |
|------|:----:|
| ToolSearchEngine + find_tool 元工具（完整搜索：exact/prefix/fuzzy/fulltext + 别名 + 分类过滤 + 频次排序） | ❌ |
| ToolOutput 统一信封增强（meta/confirm/recoverable 字段） | ❌ |
| 自动 Input Schema 校验 | ❌ |
| 全工具描述英文化 | ❌ |
| `completion/complete` 集成 ToolSearchEngine | ❌ |

### Phase 1 — Undo/Redo + SDK 平权

| 任务 | 状态 |
|------|:----:|
| supports_undo/is_destructive 能力声明 | ❌ |
| HandlerRegistry 自动 Undo 包装 | ❌ |
| IToolAdapter（SDK → ITool，SDK 工具注册到 itool_table_ 主表） | ❌ |
| SDK 工具可通过属性声明 needs_scene/needs_node/supports_undo | ❌ |

### Phase 2 — 三层工具体系

| 任务 | 状态 |
|------|:----:|
| 通用兜底工具：get/set_node_property, get/set_resource_property, call_node_method, run_editor_script | ❌ |
| 复合工具：edit_transform, edit_node, edit_resource, batch_set_property, duplicate_nodes, create_scene_skeleton | ❌ |
| 分类系统 YAML 驱动取代 top_level_meta() 硬编码 | ❌ |

### Phase 3 — MCP 差异化

| 任务 | 状态 |
|------|:----:|
| MCP Resources：godot://scene/current, godot://project/config | ❌ |
| MCP Prompts：create_player_controller, debug_performance, setup_collision_shapes | ❌ |
