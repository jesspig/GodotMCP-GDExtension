# Getting Started

## Requirements

- **Godot 4.6+** (editor, not export templates)
- **AI client** that supports MCP Streamable HTTP (Claude Code, Cline, Continue, Cursor, opencode, Roo Code, etc.)

## Installation

### 1. Download the Plugin

Download the latest `addons.zip` from the [Releases](https://github.com/JessPig/GodotMCP-GDExtension/releases) page.

### 2. Extract to Your Project

Extract the zip into your Godot project root so that the plugin files are at:

```
your_project/
addons/
  godot_mcp/
    plugin.cfg
    godot_mcp.gdextension
    bin/
      godot_mcp_gdext.dll    # Windows
      libgodot_mcp_gdext.so  # Linux
      libgodot_mcp_gdext.dylib  # macOS
```

### 3. Enable the Plugin

1. Open your project in Godot 4.6+
2. Go to **Project > Project Settings > Plugins**
3. Find **Godot MCP** and set the status to **Enable**

### 4. Verify It Works

Check the Godot editor output console for:

```
GodotMCP: HTTP server started on port 9600
```

Or test with curl:

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_info","arguments":{}}}'
```

Expected response:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{
      "type": "text",
      "text": "{\"success\":true,\"data\":{\"connection\":{\"status\":\"ok\"},\"engine\":{...}}}"
    }]
  }
}
```

## Configure Your AI Client

See [Client Setup](/guide/client-setup) for configuring opencode, Cursor, Claude Code, Cline, and other AI tools.

## What's Next?

- Read the [Tools Overview](/guide/tools-overview) to see what you can do
- Browse the [Tools Reference](/reference/meta-tools) for detailed tool parameters
- Check the [FAQ](/guide/faq) for common issues