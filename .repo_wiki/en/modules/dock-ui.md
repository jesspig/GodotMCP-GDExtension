# Dock UI

> Godot editor right-side panel.

```mermaid
flowchart TD
    subgraph DOCK["Right-side Dock Panel"]
        TAB["TabContainer"]
        
        subgraph TAB1["Status (StatusBar)"]
            S_CONN["Connection status indicator"]
            S_TOOL["Current tool"]
            S_LAT["Latency / response time"]
        end
        
        subgraph TAB2["Integration (Integration)"]
            I_SET["Config editor"]
            I_TEST["Connection test button"]
        end
        
        subgraph TAB3["Settings (Settings)"]
            SET_PORT["Port config"]
            SET_SAVE["Save button"]
        end
        
        subgraph TAB4["Tool Manager (ToolManager)"]
            T_LIST["Available tools list"]
            T_SEARCH["Filter search"]
        end
    end
    
    DOCK -->|add_control_to_dock| EDITOR["EditorInterface"]
    
    TAB --> TAB1
    TAB --> TAB2
    TAB --> TAB3
    TAB --> TAB4
```

## Implementation

Dock UI is partially implemented with some **markers still TODO**.

### `main_dock.rs`

- Creates `VBoxContainer` as root control
- Instantiates 4 sub-panels

### Sub-panels

| File | Status |
|------|--------|
| `status_bar.rs` | Implemented: connection status, last tool call, latency |
| `integration.rs` | Implemented: config validation + test button |
| `settings.rs` | Implemented: WebSocket port config + persistence |
| `tool_manager.rs` | Marked TODO |

## Connections

- Dock UI references `PluginState::global().editor_interface` for `EditorInterface`
- Settings persisted via `ProjectSettings`

## Future Plans

- `tool_manager.rs` will display available MCP tools (calls `list_request_tools` on server)
- Tool search and filtering
- Test tool calls directly from UI with JSON response display
