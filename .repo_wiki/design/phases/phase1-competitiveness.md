# Phase 1 — P1 竞争力提升

> 预计工期：2-3 周。不做则在竞品中明显落后。
> ADR-016 子决策：决策 4（编辑器 UI 面板）、决策 5（工具覆盖面补全）、决策 6（WSL2 兼容）、决策 7（安全模型）。

## P1-1：编辑器底部面板 UI

### 背景

竞品 satelliteoflove/godot-mcp 提供了编辑器底部面板 UI，显示 MCP 连接状态、端口配置、日志输出。本项目当前零 UI 控件。

godot-cpp 10.0.0-rc1 的 `add_control_to_bottom_panel` 等 API **已完整绑定**（AGENTS.md 中"未绑定需 call() 兜底"的信息已过时），技术上无障碍。

### 设计方案

```
┌─────────────────────────────────────────────────────────────────┐
│ [Godot MCP]  ● Running  |  HTTP :9600  |  Tools: 149 (3 custom) │
│ Bridge: ● Connected (:9601)                                       │
│ ┌─────────────────────────────────────────────────────────────────┐│
│ │ [Activity]                                       [Clear]        ││
│ │ 12:01:23 tools/call: add_node -> ok                           ││
│ │ 12:01:25 tools/call: set_node_property -> ok                  ││
│ │ 12:01:28 tools/call: capture_viewport -> ok                   ││
│ └─────────────────────────────────────────────────────────────────┘│
│ [Settings]                                                        │
│ HTTP Port: [9600 ▾]   Bridge Port: [9601 ▾]   [Restart]         │
│ Bind: (●) Localhost  ( ) WSL  ( ) Custom [___________]          │
└─────────────────────────────────────────────────────────────────┘
```

### 修改清单

#### 1. 新增 `extensions/src/ui/mcp_panel.hpp` + `mcp_panel.cpp`

```cpp
class McpPanel : public Control {
    GDCLASS(McpPanel, Control)

    // 子控件
    Label *status_label_;          // "● Running" / "● Stopped"
    Label *tools_count_label_;     // "Tools: 149 (3 custom)"
    Label *bridge_status_label_;   // "Bridge: ● Connected (:9601)"
    RichTextLabel *activity_log_;  // 最近 50 条工具调用日志
    SpinBox *http_port_spin_;      // HTTP 端口配置
    SpinBox *bridge_port_spin_;    // 桥接端口配置
    Button *restart_btn_;          // 重启服务器
    OptionButton *bind_mode_;      // Localhost / WSL / Custom
    LineEdit *custom_bind_addr_;   // 自定义绑定地址

    // 状态更新（每秒节流，非每帧）
    double time_since_update_ = 0.0;
    void update_status();

protected:
    static void _bind_methods();

public:
    McpPanel();
    ~McpPanel();
    void _process(double delta) override;
};
```

**功能清单**：
- 连接状态指示灯（绿色 = 运行，红色 = 停止）
- 内置/自定义工具数
- 桥接状态 + 端口
- 活动日志（RichTextEdit，滚动显示最近 50 条工具调用）
- 端口配置（SpinBox，1-65535）
- 绑定模式选择（Localhost / WSL / Custom）
- 重启按钮

#### 2. 修改 `extensions/src/editor_plugin.cpp`

```cpp
void McpEditorPlugin::_enter_tree() {
    // ... 现有初始化代码 ...

    panel_ = memnew(McpPanel);
    add_control_to_bottom_panel(panel_, "Godot MCP");
}

void McpEditorPlugin::_exit_tree() {
    remove_control_from_bottom_panel(panel_);
    memdelete(panel_);
    panel_ = nullptr;

    // ... 现有清理代码 ...
}
```

#### 3. 修改 `extensions/CMakeLists.txt`

源码列表新增：
```cmake
src/ui/mcp_panel.cpp
```

#### 4. 修改 `extensions/src/editor_plugin.hpp`

新增成员：
```cpp
class McpPanel;
McpPanel *panel_ = nullptr;
```

### 验证标准

1. Godot 编辑器底部出现 "Godot MCP" 标签页
2. 点击展开可看到实时状态（HTTP 运行/停止、工具数、桥接状态）
3. 修改端口后点击 Restart 按钮，MCP 服务在新端口可用
4. 工具调用实时显示在活动日志中

---

## P1-2：关键工具领域补全

### 背景分析

对照竞品工具覆盖矩阵，本项目在以下领域完全缺失：

| 领域 | 缺失影响 | 新增工具数 | 估计代码量 |
|------|---------|:---------:|:---------:|
| **3D 碰撞** | 3D 游戏开发阻断 | 1（通用） | ~200 行 |
| **AnimationTree** | 复杂动画系统阻断 | 5-7 | ~300 行 |
| **Audio** | 游戏音效/背景音乐阻断 | 3-4 | ~200 行 |
| **Navigation** | AI 寻路系统缺失 | 3-4 | ~200 行 |
| **3D 场景构建** | 语义化快捷操作缺失 | 3-4 | ~250 行 |
| **Shader Uniform** | 材质参数调试缺失 | 2 | ~100 行 |
| **Export 管理** | CI/CD 流水线缺失 | 2-3 | ~150 行 |
| **InputMap 写操作** | 输入配置缺失 | 3 | ~100 行 |

### 实现策略

**每个新工具是一个 `.hpp` 文件（~50-80 行），放在 `extensions/src/built_in/tools/<category>/` 下。**

注册步骤：
1. 创建 `.hpp` 文件实现 `ITool` 接口
2. 在 `extensions/src/built_in/tools/register/register_existing.hpp` 加一行 `GODOT_MCP_TOOL` 宏
3. 在 `extensions/src/built_in/register_itools.cpp` 加一行 `#include`

#### 第一批（核心工作流阻断，优先实现）

##### 3D 碰撞 — `create_collision_shape_3d.hpp`

```cpp
// 参数：dimension="3d", body_type, shape_type, size/radius/height/extents, parent_path, node_name
// 功能：一步创建 PhysicsBody3D + CollisionShape3D + 配置形状
// 支持：BoxShape3D, SphereShape3D, CapsuleShape3D, CylinderShape3D, ConvexPolygon3D, ConcavePolygon3D
//       StaticBody3D, RigidBody3D, CharacterBody3D, Area3D
```

参考现有 `create_collision_shape.hpp`（2D 版本），扩展为同时支持 2D 和 3D。

##### AnimationTree — 5 个新工具

| 工具 | 功能 |
|------|------|
| `create_animation_tree` | 创建 AnimationTree 节点 + 关联 AnimationPlayer |
| `get_animation_tree_info` | 查询 AnimationTree 结构（状态机、混合树、转换） |
| `add_animation_node` | 向状态机/混合树添加动画节点 |
| `add_transition` | 添加状态转换 + 设置条件 |
| `set_transition_condition` | 设置/修改转换条件（表达式） |

##### Audio — 3 个新工具

| 工具 | 功能 |
|------|------|
| `create_audio_player` | 创建 AudioStreamPlayer / AudioStreamPlayer2D / AudioStreamPlayer3D |
| `set_audio_stream` | 设置音频流资源路径 |
| `list_audio_buses` | 列出音频总线布局 |

#### 第二批（补全工作流）

##### Navigation — 3 个工具

| 工具 | 功能 |
|------|------|
| `create_navigation_region` | 创建 NavigationRegion2D / NavigationRegion3D |
| `create_navigation_agent` | 创建 NavigationAgent2D / NavigationAgent3D |
| `bake_navigation_mesh` | 触发导航网格烘焙 |

##### 3D 场景构建 — 3 个工具

| 工具 | 功能 |
|------|------|
| `create_mesh_instance_3d` | 创建 MeshInstance3D + 配置 Mesh（Box/Sphere/Cylinder/Capsule/Plane/导入 .glb） |
| `create_light_3d` | 创建 DirectionalLight3D / OmniLight3D / SpotLight3D |
| `set_world_environment` | 创建/配置 WorldEnvironment + Sky + Environment |

##### Shader Uniform — 2 个工具

| 工具 | 功能 |
|------|------|
| `get_shader_uniforms` | 查询 Shader 的 uniform 参数列表（名称、类型、默认值） |
| `set_shader_uniform` | 设置 ShaderMaterial 的 uniform 参数值 |

##### Export 管理 — 2 个工具

| 工具 | 功能 |
|------|------|
| `validate_export_presets` | 验证导出配置完整性（模板、平台、资源过滤） |
| `get_export_platforms` | 列出可用导出平台和已安装模板状态 |

##### InputMap 写操作 — 3 个工具

| 工具 | 功能 |
|------|------|
| `add_input_action` | 添加新输入动作 + 设置死区 |
| `remove_input_action` | 删除输入动作 |
| `add_input_event_binding` | 为动作添加输入事件绑定（键/鼠标/手柄） |

### 验证标准

每个新工具附带一个 YAML 测试用例，验证基本功能。

---

## P1-3：WSL2 支持

### 背景

在"Windows Godot + WSL2 AI Client"这个常见开发场景下，WSL2 内的 AI 客户端无法直接访问 Windows 侧的 `127.0.0.1:9600`。竞品 satelliteoflove/godot-mcp 通过 Bind Mode UI + 自动检测解决了此问题。

### 设计方案

#### 1. 动态绑定地址

`HttpServer::start()` 接受动态绑定地址，取代硬编码 `kBindAddress`。

```cpp
// http_server.hpp — 修改前
static constexpr const char *kBindAddress = "127.0.0.1";

// http_server.hpp — 修改后
String bind_address_ = "127.0.0.1";
Error start(uint16_t port, const String &bind_address = "127.0.0.1");
```

#### 2. 环境变量配置

新增环境变量 `GODOT_MCP_HTTP_HOST`：
- 默认：`127.0.0.1`（仅本地回环）
- WSL 场景：`0.0.0.0`（绑定所有接口）

#### 3. 自动检测

在 `editor_plugin.cpp` 中检测 WSL2 环境（仅 Windows 平台）：

```cpp
String http_host = "127.0.0.1";
// WSL 模式：检查环境变量显式请求
String env_host = OS::get_singleton()->get_environment("GODOT_MCP_HTTP_HOST");
if (!env_host.is_empty()) {
    http_host = env_host;
}
```

#### 4. UI 集成

底部面板（P1-1）中的 Bind Mode 选择器：
- **Localhost**（默认）：`127.0.0.1`
- **All Interfaces**（WSL/远程）：`0.0.0.0`
- **Custom**：用户输入 IP 地址

#### 5. 文档

新增 WSL2 配置指南到 `docs/` 文档站。

### 修改清单

| 文件 | 修改 |
|------|------|
| `extensions/src/server/ipc/http_server.hpp` | `kBindAddress` → 动态参数 |
| `extensions/src/server/ipc/http_server.cpp` | `listen()` 使用动态地址 |
| `extensions/src/editor_plugin.cpp` | 读取 `GODOT_MCP_HTTP_HOST` 环境变量 |
| `extensions/src/ui/mcp_panel.hpp/cpp` | Bind Mode UI |
| `docs/` | WSL2 配置指南 |

### 验证标准

1. WSL2 内执行 `curl http://$(hostname).local:9600/mcp` 可连接到 Windows 侧的 MCP 服务
2. Localhost 模式下 `netstat` 确认仅 127.0.0.1 绑定
3. All Interfaces 模式下 `netstat` 确认 0.0.0.0 绑定

---

## P1-4：CORS 安全加固 + Session 生命周期

### CORS 矛盾修复

**现状**：Origin 校验（白名单：localhost/127.0.0.1/::1）与 `Access-Control-Allow-Origin: *` 矛盾。

**修改** (`http_connection.cpp`)：

```cpp
// 修改前
write_header("Access-Control-Allow-Origin", "*");

// 修改后
String origin = request_headers_.get("origin", "");
if (is_allowed_origin(origin)) {
    write_header("Access-Control-Allow-Origin", origin);
    write_header("Vary", "Origin");
} else {
    write_header("Access-Control-Allow-Origin", "null");
}
```

### Session 生命周期管理

**现状**：Session 永不过期且无上限，`sessions_` 是 `HashMap<String, Session>` 无容量限制。

**修改** (`mcp_handler.hpp/cpp`)：

```cpp
// Session 结构新增
double last_activity = 0.0;     // OS::get_ticks_msec() / 1000.0
static constexpr double kSessionTtl = 3600.0;  // 1 小时
static constexpr int kMaxSessions = 16;

// 定期清理（在 poll() 中或每次 handle_message 时惰性清理）
void cleanup_expired_sessions() {
    double now = OS::get_singleton()->get_ticks_msec() / 1000.0;
    // 删除 TTL 过期的 session
    // 超过上限时淘汰最旧的 session
}
```

### 验证标准

1. 跨域请求（非白名单 Origin）被 CORS 拒绝
2. 白名单内 Origin 正确反射
3. Session 1 小时无活动自动清理
4. 超过 16 个 session 时最旧的被淘汰
