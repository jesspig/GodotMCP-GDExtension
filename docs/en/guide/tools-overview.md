# Tools Overview

GodotMCP provides **153 built-in tools** organized into a four-layer system. Tools are auto-discovered by category and searchable via the `find_tool` meta tool.

## Four-Layer System

| Layer | Count | Purpose |
|-------|-------|---------|
| Layer 0: Generic Fallback | 2 | Universal property get/set for any node |
| Layer 1: Meta Tools | 7 | Tool discovery, introspection, configuration |
| Layer 2: Semantic Tools | 136 | Purpose-built tools for every editor domain |
| Layer 3: Doc Query Tools | 8 | ClassDB-powered Godot API reference |

## Category Tree

```
meta_tools/                       # 7 tools
  get_info, get_tools, get_categories, get_tool_detail,
  find_tool, call_tool, generate_client_config

node_tools/
  signal/                         # 4 tools
  group/                          # 4 tools
  general/                        # 6 resource tools
  fallback/                       # 2 property tools

editor_tools/
  scene_tree/                     # 24 tools
  scripts/                        # 12 tools (GD + C#)
  filesystem/                     # 12 tools
  workspace/                      # 13 tools
  animation/                      # 10 tools
  settings/                       # 4 tools
  shader/                         # 5 tools
  control/                        # 4 tools
  inputmap/                       # 4 tools
  export/                         # 4 tools
  docs/                           # 8 tools
  audio/                          # 3 tools
  navigation/                     # 3 tools
  3d_scene/                       # 3 tools
  tilemap/                        # 3 tools
  plugin/                         # 2 tools
  collision/                      # 1 tool
  scaffold/                       # 1 tool
  visualizer/                     # 1 tool

runtime_tools/
  bridge/                         # 7 tools
  lifecycle/                      # 6 tools
```

## Finding Tools

Use the `find_tool` meta tool to search by name, description, or category:

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"find_tool","arguments":{"query":"animation"}}}'
```

Use `get_tools` to list all tools, or `get_categories` to browse the category tree.

## How Tools Work

Every tool follows the same pattern:

1. **Input**: JSON-RPC `tools/call` with tool name and arguments
2. **Execution**: Server runs the tool and returns a result
3. **Output**: `{success: true, data: {...}}` or `{success: false, error: {code, message}}`

All tools have an **input schema** that describes their expected parameters. Clients use this schema to auto-generate tool call forms.