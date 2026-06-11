# 测试编排器

> `test_orchestrator.py`——管理 Godot 编辑器生命周期、运行所有测试阶段、生成报告。

## 生命周期

```mermaid
sequenceDiagram
    participant O as test_orchestrator.py
    participant FS as 文件系统
    participant GM as GodotManager
    participant MC as McpSession
    participant G as Godot 编辑器
    
    O->>FS: backup_example() → 备份 icon.svg / .gitignore / project.godot
    O->>GM: ensure_running()
    GM->>G: 启动 godot --editor --headless
    G->>G: 加载 gdext → _enter_tree() → HTTP :9600
    loop 轮询
        GM->>G: GET /mcp
        G-->>GM: 200 OK
    end
    GM-->>O: Godot 就绪
    
    O->>MC: create_mcp_client()
    MC->>G: POST /mcp (initialize)
    G-->>MC: Session-ID + capabilities
    MC->>G: POST /mcp (tools/list)
    G-->>MC: 122 tools discovered
    MC-->>O: 会话就绪

    loop 18 个阶段
        O->>O: 运行 PhaseRunner.run_all()
        O->>MC: call_tool(tool_name, args)
        MC->>G: POST /mcp (tools/call)
        G-->>MC: ToolResult
        MC-->>O: 解析后的结果
        O->>FS: file_verifier 磁盘验证
    end

    O->>GM: stop()
    GM->>G: kill process
    O->>FS: restore backup files
    O->>O: save_json + save_markdown 报告
```

## 关键函数

| 函数 | 说明 |
|------|------|
| `load_config()` | 从 `.env` 加载环境变量，计算路径 |
| `backup_example()` | 备份 `icon.svg`、`.gitignore`、`project.godot` 到 `tests/backup/` |
| `run_test_session()` | 异步主流程：启动 → 连接 → 执行 → 停止 → 报告 |
| `_restore_fallback()` | 清理阶段未运行时回退恢复文件 |
| `main()` | 入口点，验证 `GODOT_PATH`，异常退出码 1 |

## 路径约定

| 路径 | 用途 |
|------|------|
| `tests/backup/` | 预测试备份目录 |
| `tests/output/` | 报告输出目录 |
| `tests/.env` | 环境配置 |
| `example/` | Godot 测试项目根目录 |
