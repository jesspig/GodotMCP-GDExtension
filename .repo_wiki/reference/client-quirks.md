# Client Configuration Quirks Cheatsheet

> Per-client gotchas worth memorising before editing any MCP config. Templates live in [client-config.md](client-config.md).

| Client | Quirk | Bites because |
|--------|-------|---------------|
| **GitHub Copilot** | Top-level key is `servers`, **not** `mcpServers`. Every entry needs an explicit `"type": "stdio"` or `"type": "http"`. | A `mcpServers` block is silently ignored; the tool never appears in Copilot's tool list and there's no error message. |
| **Antigravity** | HTTP URL key is `serverUrl`, not `url`. | Antigravity refuses to start the entry but the error message just says "missing URL". |
| **Gemini CLI / Qwen Code** | HTTP URL key is `httpUrl`. `url` is reserved for legacy SSE. | Picking the wrong key falls back to SSE, which our server never speaks → silent connection failure. |
| **Codex** | Config file is `~/.codex/config.toml` — **TOML, not JSON**. Entries live under `[mcp_servers."<id>"]`. | Easy to drop a JSON block here and never get a tool list. |
| **Trae CN** | Uses the same `<project>/.trae/mcp.json` as Trae. | If both clients are installed, they share config. |
| **Qoder** | Config path is OS-specific: Win `%APPDATA%/Qoder/mcp-settings.json` · mac `~/Library/Application Support/Qoder/` · Linux `~/.config/qoder/`. | Easy to put it in the wrong place when copying instructions. |
| **CodeBuddy** | Every entry needs explicit `"type"` (`"stdio"` / `"sse"` / `"http"`). | Without it the entry is rejected. |
| **OpenCode** | Accepts both `~/.config/opencode/config.json` and per-project `opencode.json`. Project file wins. | If a stale global entry exists, you'll think your project edit took effect when it didn't. |
| **Cursor** | Project-level only (`<project>/.cursor/mcp.json`). | No user-level fallback; the file must exist *next to the project root*. |

## When in doubt

- `ping` is the safe smoke-test tool — every client that can list tools can call it.
- Most clients hot-reload the config; some (Trae, Qoder) require restart. If a config change "doesn't take", restart the client first.
- The server logs every received call to stderr (`[Godot MCP] Received: …`). If you see it in stderr but not in the client UI, the client's transport is mis-wired (e.g. it's expecting HTTP but sending stdio).
