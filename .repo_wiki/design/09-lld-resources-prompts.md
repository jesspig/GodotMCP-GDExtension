# Resources & Prompts 增强 — 底层设计

> 版本: 1.0 · 2026-06-20  
> 对应迭代: Phase 2 (`feature/capability-catchup`)  
> 状态: 草案

---

## 1. 现状与问题

### 1.1 Resources 现状

当前实现了 3 个静态 Resources + 1 个 URI Template：

| Resource | 实现 | 覆盖度 |
|----------|------|:------:|
| `godot://scene-tree` | `mcp_handler.cpp:452-473` | 基础场景树结构 |
| `godot://project-settings` | `mcp_handler.cpp:475-498` | 仅 8 个字段，严重不足 |
| `godot://editor-info` | `mcp_handler.cpp:500-522` | 基础信息 |
| `godot://scene-node/{path}` | `mcp_handler.cpp:420-450` | URI 模板，按路径查询 |

**问题**：
- 大量只读操作仍走 `tools/call`，浪费了 MCP 协议的 Resource 语义（无副作用、可缓存、非消耗性）
- `project-settings` 仅返回 8 个预设字段，用户无法获取任意配置
- 无日志/断点/性能/文件系统等常用只读数据的 Resource
- Resources 没有配套测试

### 1.2 Prompts 现状

当前 5 个 Prompt：

| Prompt | 实现 | 类型 |
|--------|------|:----:|
| `create_scene` | `prompt_provider.cpp:21-43` | 静态文本模板 |
| `create_node` | `prompt_provider.cpp:45-59` | 静态文本模板 |
| `fix_error` | `prompt_provider.cpp:61-77` | 静态文本模板 |
| `explain_node` | `prompt_provider.cpp:79-95` | 静态文本模板 |
| `code_review` | `prompt_provider.cpp:97-115` | 静态文本模板 |

**问题**：
- 全部为硬编码文本模板，无运行时数据注入（如 `explain_node` 无法实际查询 ClassDB）
- 无多步工作流指导（如"创建场景 → 添加节点 → 挂载脚本 → 测试"的完整过程）
- 无参数联动（`fix_error` 的 `error_text` 应该能从 `get_console_output` 自动获取）
- Prompts 缺乏工具调用链接的语义提示

---

## 2. Resources 增强设计

### 2.1 设计原则

| 原则 | 说明 |
|------|------|
| **只读操作优先** | 凡是只有读操作、无副作用的工具调用，优先转换为 Resource |
| **分层缓存** | 高频资源（scene-tree）可缓存 ~500ms，低频资源（class/console）实时查询 |
| **URI 参数标准化** | 所有 URI 模板使用 Godot 路径约定，`{path}` 用 NodePath 格式 |
| **向后兼容** | 工具保留不下线，Resource 优先；用户可任选一种方式 |

### 2.2 增强后 Resources 清单

```
godot://scene-tree?max_depth=3&max_children=200   # 场景树（已有，增强查询参数）
godot://project-settings?filter=display/window     # 项目设置（扩展：可过滤）
godot://editor-info                                # 编辑器信息（已有）
godot://scene-node/{path}                          # 单节点详情（已有 URI 模板）
godot://console?lines=50&source=editor             # 控制台输出（新增）
godot://breakpoints                                # 断点列表（新增）
godot://performance?filter=physics                 # 性能监视器（新增）
godot://filesystem/{path}?pattern=*.tscn           # 文件系统浏览（新增）
godot://signals/{node_path}                        # 节点信号列表（新增）
godot://groups/{node_path}                         # 节点分组列表（新增）
godot://class/{name}                               # ClassDB 类文档（新增）
godot://classes?search=node                        # ClassDB 类列表（新增）
```

| Resource | 替代工具 | 预期用途 |
|----------|---------|----------|
| `godot://console` | `get_console_output` | AI 先读控制台再修复错误 |
| `godot://breakpoints` | `list_breakpoints` | AI 阅读调试状态 |
| `godot://performance` | `get_performance_monitors` | 性能分析前提 |
| `godot://filesystem/{path}` | `list_directory` | 浏览项目文件先于操作 |
| `godot://signals/{path}` | `list_signals` | 了解信号后连接 |
| `godot://groups/{path}` | `get_node_groups` | 了解分组后操作 |
| `godot://class/{name}` | `get_class_info` | 学习 API 后使用 |
| `godot://classes` | `get_class_list` | 搜索可用类 |

**消除的工具**（使用 Resource 替代后，工具可以保留但 AI 更倾向使用 Resource）：
- `get_console_output`, `list_breakpoints`, `get_performance_monitors`
- `list_directory`, `list_signals`, `get_node_groups`, `get_class_info`, `get_class_list`

### 2.3 实现模式

每个 Resource handler 遵循统一模板：

```cpp
// 伪代码模式
if (uri == "godot://console") {
    int lines = params.get("lines", 50);
    // 从 EditorInterface / EditorLog 读取
    Array contents;
    Dictionary text_item;
    text_item["type"] = "text";
    text_item["text"] = get_console_text(lines);
    contents.append(text_item);
    return make_resource_result(contents);
}
```

### 2.4 迁移策略

| 阶段 | 动作 |
|:----:|------|
| Phase 2 | 新增 8 个 Resources + URI 模板，工具保持可用 |
| Phase 3 | 文档中推荐 AI 使用 Resource 替代只读工具调用 |
| Phase 4 | 可选废弃只读工具，仅保留 Resource 路径 |

---

## 3. Prompts 增强设计

### 3.1 设计原则

| 原则 | 说明 |
|------|------|
| **工作流导向** | Prompt 不应只是"创建场景"的知识点，而是"创建场景 → 添加节点 → 挂载脚本 → 测试"的完整链路 |
| **运行时数据注入** | `explain_node` 应实际调用 ClassDB 查询，非硬编码文本 |
| **工具链接提示** | Prompt 末尾提示后续可用的工具序列（如"创建完成后可用 `run_project` 测试"） |
| **故障排除** | `fix_error` 应包含 grep 控制台 + 定位根因 + 修复 + 验证的标准流程 |

### 3.2 增强后 Prompts 清单

基础 Prompt（已有 5 个增强 + 新增 4 个工作流 Prompt）：

```cpp
// 已有 Prompts（增强）
{ "create_scene",  "多步工作流：选择类型 → 创建节点 → 设置属性 → 保存" },
{ "create_node",   "多步工作流：选择父节点 → 选择类型 → 配置属性 → 验证" },
{ "fix_error",     "全链路：读取控制台 → 定位错误 → 修复 → 验证" },
{ "explain_node",  "动态查询：通过 ClassDB 实时获取节点 API 文档" },
{ "code_review",   "全链路：读取脚本 → 静态分析 → 建议 → 展示 diff" },

// 新增 Prompts
{ "debug_session",  "工作流：设置断点 → 启动游戏 → 触发断点 → 读取变量 → 步进" },
{ "animate_node",   "工作流：创建 AnimationPlayer → 添加轨道 → 设置关键帧 → 预览" },
{ "shadow_edit",    "工作流：暂存快照 → 编辑 → 预览 diff → apply/rollback" },
{ "export_project", "工作流：配置导出预设 → 验证 → 构建发布" },
```

### 3.3 `explain_node` — 动态 ClassDB 查询

当前行为：返回一段硬编码文本模板。
目标行为：实际查询 Godot ClassDB 获取最新文档。

```cpp
Dictionary make_explain_node_prompt(const Dictionary &args) {
    String node_type = args.get("node_type", "Node");

    // 通过 ClassDB 获取实时信息
    EditorInterface *ei = EditorInterface::get_singleton();
    String class_doc = ei->get_editor_docs()->get_class_doc(node_type);
    String inheritance = get_inheritance_chain(node_type);
    Array methods = get_method_list(node_type);
    Array properties = get_property_list(node_type);

    String prompt_text = "The " + node_type + " node:\n"
        "- Inheritance: " + inheritance + "\n"
        "- Key properties: " + join_properties(properties) + "\n"
        "- Available methods: " + join_methods(methods) + "\n"
        "\nUse get_tool_detail to find tools that work with this node type.";

    // 返回结构化的 prompt 内容
    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Real-time ClassDB documentation for " + node_type;
    result["messages"] = messages;
    return result;
}
```

### 3.4 `fix_error` — 控制台集成

```cpp
Dictionary make_fix_error_prompt(const Dictionary &args) {
    String error_text = args.get("error_text", "");

    // 自动附加控制台上下文
    String console_context = get_console_output(20); // 最近 20 行

    String prompt_text = "Error: " + error_text + "\n\n"
                         "Console context (last 20 lines):\n" + console_context + "\n\n"
                         "Suggested diagnostic workflow:\n"
                         "1. Use read_script on the file containing the error\n"
                         "2. Use get_node_property to inspect suspect node state\n"
                         "3. Apply fix with set_node_property or patch_script\n"
                         "4. Verify by re-running the scene";

    Array messages;
    messages.append(make_text_message(prompt_text));

    Dictionary result;
    result["description"] = "Error analysis with live console context";
    result["messages"] = messages;
    return result;
}
```

### 3.5 `debug_session` — 全新工作流 Prompt

```cpp
Dictionary make_debug_session_prompt(const Dictionary &args) {
    String target_script = args.get("script_path", "");
    String prompt_text =
        "Debug session workflow for " + target_script + ":\n\n"
        "1. Set breakpoints using set_breakpoint at key lines\n"
        "2. Run the project or current scene\n"
        "3. Trigger the breakpoint by interacting with the game\n"
        "4. Use get_stack_trace to examine the call stack\n"
        "5. Use get_locals to inspect variable values\n"
        "6. Use debugger_control to step over/into\n"
        "7. Fix any issues with patch_script\n"
        "8. Run again to verify the fix";

    // ...
}
```

---

## 4. 改动量估算

### 4.1 Resources 增强

| 文件 | 改动 | 行数 |
|------|------|:----:|
| `server/mcp/mcp_handler.hpp` | 新增 Resource handler 方法声明 | ~10 |
| `server/mcp/mcp_handler.cpp` | 新增 8 个 Resource handler + 更新 `handle_resources_list` + 更新 `handle_resources_templates_list` | ~200 |
| `testing/yaml_tests/*.yaml` | 新增 Resources 测试用例 | ~40 |
| **合计** | | **~250** |

### 4.2 Prompts 增强

| 文件 | 改动 | 行数 |
|------|------|:----:|
| `server/mcp/prompt_provider.cpp` | 增强 3 个已有 Prompt + 新增 4 个 Workflow Prompt | ~120 |
| **合计** | | **~120** |

**总计**: ~370 行新增，零外部依赖，零架构变更。

---

## 5. 验收标准

1. 所有 8 个新增 Resources 可通过 `resources/list` + `resources/read` 正确访问
2. `godot://project-settings?filter=*` 支持任意 ProjectSettings key 查询（非仅 8 个预设）
3. `explain_node` 返回的内容实时来源于 ClassDB，非硬编码文本
4. `fix_error` 自动包含控制台最后 N 行上下文
5. 4 个新增工作流 Prompt 可在 `prompts/get` 正确获取
6. 所有已有测试通过
7. AI 可通过 `prompts/list` 发现所有 9 个 Prompt

---

## 6. 与现有架构的关系

```
Resources 增强
  └─ 复用现有的 handle_resources_list / handle_resources_read 路由
  └─ 新增 handler 遵循已有 _build_scene_tree_node 模式
  └─ 不影响 tools/call 路径

Prompts 增强
  └─ 复用现有的 prompt_provider::list_prompts / get_prompt 接口
  └─ 新增 factory 函数遵循已有 PromptEntry 模式
  └─ 运行时注入通过已有的 args.get() 机制
```

零架构侵入——两个增强都不需要修改核心路由、不增加新工具、不改变协议行为。
