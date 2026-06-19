# 测试与质量保证策略

## 1. 测试金字塔

```
        ┌─────────┐
        │  E2E    │  ← MCP Inspector 手动测试
        │  (少量) │
       ┌┴─────────┴┐
       │ Integration │  ← TestEngine YAML 流水线
       │   (中量)    │
      ┌┴─────────────┴┐
      │   Unit Tests   │  ← C++ 单元测试
      │    (大量)      │
      └────────────────┘
```

## 2. 单元测试

### 2.1 WebSocket 帧编解码

```yaml
# tests/yaml_tests/ws_codec.yaml
name: "WebSocket Codec Unit Tests"
headless: true
stages:
  - id: encode_text
    steps:
      - id: encode_small
        tool: _test_ws_encode  # 需要暴露为内部测试工具
        args:
          opcode: 1
          payload: "Hello"
        expect:
          success: true
          frame_length: 7  # 2 header + 5 payload

  - id: decode_text
    steps:
      - id: decode_small
        tool: _test_ws_decode
        args:
          hex_data: "810548656c6c6f"  # FIN + TEXT + "Hello"
        expect:
          success: true
          opcode: 1
          payload: "Hello"

  - id: roundtrip
    steps:
      - id: encode_decode
        tool: _test_ws_roundtrip
        args:
          text: '{"type":"event","event":"metrics","data":{"fps":60}}'
        expect:
          success: true
          decoded_matches: true
```

### 2.2 Operation ID 唯一性

```yaml
# tests/yaml_tests/op_id_uniqueness.yaml
name: "Operation ID Uniqueness"
headless: true
stages:
  - id: generate_ids
    steps:
      - id: gen_100_ids
        tool: _test_generate_operation_ids
        args:
          count: 100
        expect:
          success: true
          unique_count: 100  # 所有 ID 唯一
```

### 2.3 渐进式披露开关

```yaml
# tests/yaml_tests/progressive_disclosure.yaml
name: "Progressive Disclosure Toggle"
headless: true
stages:
  - id: progressive_mode
    steps:
      - id: list_tools_progressive
        tool: list_tools
        args:
          mode: progressive
        expect:
          success: true
          tool_count: 8  # 仅 meta 工具

  - id: full_mode
    steps:
      - id: list_tools_full
        tool: list_tools
        args:
          mode: full
        expect:
          success: true
          tool_count_minimum: 150  # 全部工具
```

## 3. 集成测试

### 3.1 MCP 协议一致性

通过 TestEngine 的 `/run-tests` 端点执行：

```yaml
# tests/yaml_tests/mcp_protocol.yaml
name: "MCP Protocol Conformance"
stages:
  - id: initialize
    steps:
      - id: init_handshake
        tool: _test_mcp_initialize
        args:
          client_name: "test-client"
          protocol_version: "2026-07-28"
        expect:
          success: true
          server_name: "godot-mcp"

  - id: tools_list
    steps:
      - id: list_meta_tools
        tool: _test_mcp_tools_list
        args: {}
        expect:
          success: true
          has_list_tools: true
          has_list_categories: true
```

### 3.2 破坏性操作确认

```yaml
# tests/yaml_tests/destructive_confirmation.yaml
name: "Destructive Operation Confirmation"
headless: false  # 需要 UI
stages:
  - id: setup
    steps:
      - id: create_scene
        tool: new_scene
        args: { root_type: "Node2D", root_name: "Test" }

  - id: destructive_op
    steps:
      - id: delete_node
        tool: delete_node
        args: { path: "." }
        expect:
          # 验证进入 pending 状态（非直接执行）
          _check_pending: true
```

### 3.3 桥接协议（Sprint 3: WebSocket）

```yaml
# tests/yaml_tests/bridge_websocket.yaml
name: "WebSocket Bridge Protocol"
headless: false  # 需要运行游戏实例
stages:
  - id: connection
    steps:
      - id: run_game
        tool: run_current_scene
        args: {}
        wait_ms: 3000

      - id: check_instances
        tool: get_game_instances
        args: {}
        expect:
          success: true
          instance_count_minimum: 1

      - id: get_scene_tree
        tool: get_game_scene_tree
        args: { instance_id: 1 }
        expect:
          success: true
          has_root: true

  - id: cleanup
    steps:
      - id: stop_game
        tool: stop_project
        args: {}
```

## 4. 回归测试

### 4.1 现有工具兼容性

每次 Sprint 完成后运行完整回归：

```bash
uv run python main.py test  # 完整测试流水线
```

### 4.2 MCP Inspector 手动测试

使用 MCP Inspector 验证端到端流程：

```bash
npx @anthropic-ai/mcp-inspector http://localhost:9600/mcp
```

测试清单：
- [ ] `tools/list` 返回正确工具列表
- [ ] `tools/call` 执行成功并返回 MCP content 格式
- [ ] 破坏性工具弹出确认对话框
- [ ] 确认后工具执行，结果通过 SSE 推送
- [ ] `rollback_operation` 正确撤销操作
- [ ] `get_game_instances` 返回已连接实例

## 5. 性能测试

### 5.1 HTTP 并发请求

```yaml
# tests/yaml_tests/http_stress.yaml
name: "HTTP Stress Test"
headless: true
stages:
  - id: concurrent_requests
    steps:
      - id: batch_call
        tool: _test_batch_tools_call
        args:
          count: 50
          tool: get_scene_tree
          args: {}
        expect:
          success: true
          all_completed: true
          max_duration_ms: 5000  # 50 请求 <5s
```

### 5.2 WebSocket 大消息

```yaml
# tests/yaml_tests/ws_large_message.yaml
name: "WebSocket Large Message"
headless: true
stages:
  - id: large_payload
    steps:
      - id: encode_64kb
        tool: _test_ws_encode_large
        args:
          size: 65536
        expect:
          success: true
          frame_valid: true
```

## 6. 质量门禁

| 门禁 | 标准 | 阻断级别 |
|------|------|---------|
| 编译通过 | 0 errors, 0 warnings（MSVC/GCC/Clang） | 阻断 |
| 现有测试全绿 | 100% pass rate | 阻断 |
| MCP Inspector | 核心流程通过 | 阻断 |
| 新增测试覆盖 | 每个新功能至少 1 个测试 | 警告 |
| 性能回归 | 无 >10% 性能下降 | 警告 |
