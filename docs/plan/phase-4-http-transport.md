# Phase 4 — HTTP Transport

> **Status**: ⏳ Not started
> **Estimate**: 3-5 days
> **Depends on**: nothing (but pairs naturally with Phase 3.3 — editable HTTP port)

## Why this phase exists

7 of the 12 supported MCP clients default to HTTP. Today only the 5 stdio-friendly clients (Claude Code, OpenCode, Codex with `type=stdio`, GitHub Copilot with `type=stdio`, CodeBuddy with `type=stdio`) can actually use the server. The HTTP rows in [`.repo_wiki/reference/client-config.md`](../../.repo_wiki/reference/client-config.md) are marked `⚠ planned`.

The good news: `rmcp = "=1.7"` is already imported with `features = ["transport-streamable-http-server"]`, and `axum = "0.8"` is in deps. Phase 4 is wiring, not new dependencies.

## What needs to change

### 4.1  CLI flag

Today `crates/server/src/main.rs` parses one option:

```rust
struct Cli {
    #[arg(long, default_value_t = 9500)]
    godot_port: u16,
}
```

Add:

```rust
#[derive(Parser, Debug)]
struct Cli {
    #[arg(long, default_value_t = 9500)]
    godot_port: u16,

    #[arg(long, value_enum, default_value_t = Transport::Stdio)]
    transport: Transport,

    #[arg(long, default_value_t = 8900)]
    http_port: u16,

    #[arg(long, default_value = "127.0.0.1")]
    http_bind: String,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum Transport { Stdio, Http, All }
```

Default stays `stdio` so existing client configs keep working with no flag changes.

### 4.2  Transport factory

Create `crates/server/src/transports/mod.rs` with:

```rust
pub async fn run_stdio(handler: GodotMcpHandler) -> anyhow::Result<()> { /* current code */ }
pub async fn run_http(handler: GodotMcpHandler, bind: SocketAddr) -> anyhow::Result<()> {
    use rmcp::transport::streamable_http_server::StreamableHttpService;

    let service = StreamableHttpService::builder()
        .service_factory({
            let handler = handler.clone();
            move || Ok(handler.clone())
        })
        .build();

    let app = axum::Router::new().nest_service("/mcp", service.into_router());
    let listener = tokio::net::TcpListener::bind(bind).await?;
    axum::serve(listener, app).await?;
    Ok(())
}
pub async fn run_all(handler: GodotMcpHandler, http_bind: SocketAddr) -> anyhow::Result<()> {
    tokio::try_join!(
        run_stdio(handler.clone()),
        run_http(handler, http_bind),
    )?;
    Ok(())
}
```

Validate that `GodotMcpHandler::clone()` still satisfies rmcp's `ServerHandler` lifetime requirements (it's `Clone` today because the registry and bridge are behind `Arc`). The shared `GodotBridge` will serialise calls naturally — only one WS connection to the editor.

### 4.3  `main.rs` switch

```rust
#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();
    let handler = GodotMcpHandler::new(cli.godot_port);
    let http_bind = format!("{}:{}", cli.http_bind, cli.http_port).parse()?;

    match cli.transport {
        Transport::Stdio => transports::run_stdio(handler).await,
        Transport::Http  => transports::run_http(handler, http_bind).await,
        Transport::All   => transports::run_all(handler, http_bind).await,
    }
}
```

### 4.4  Client-config docs

Remove the `⚠ planned, not implemented` markers from [`.repo_wiki/reference/client-config.md`](../../.repo_wiki/reference/client-config.md). Update [`reference/client-quirks.md`](../../.repo_wiki/reference/client-quirks.md) to note "HTTP works as of <date>".

### 4.5  Dock integration

If Phase 3.3 has already shipped: enable the HTTP port input. Otherwise: add a sentence "HTTP requires `--transport http` or `--transport all`."

## Risks & pitfalls

- **`StreamableHttpService` builder API surface**. rmcp 1.7 has had minor revs. Verify the exact builder signature at implementation time — if it has moved, prefer the `rmcp::transport::streamable_http_server` re-exports over deeper paths.
- **`Clone` requirements**. The `service_factory` closure runs once per HTTP session. `GodotMcpHandler` must be cheap to clone (it is — `Arc` everywhere).
- **Port conflicts**. If `8900` is in use, bail with a clear error. Don't silently fall back.
- **CORS**. rmcp's HTTP transport sets reasonable defaults but if an in-browser client ever connects, we'd need to revisit. Out of scope for now.
- **Bind address**. Default `127.0.0.1` is safe. Allowing `0.0.0.0` (via `--http-bind 0.0.0.0`) needs explicit user opt-in and we should log a warning. Auth is not in scope (loopback assumption); document the risk.

## Tests to add

- New `crates/server/tests/` integration test (currently the crate has only unit tests) that:
  - Spawns `run_http(handler, 0.0.0.0:0)`, reads the bound port back.
  - Issues an MCP `initialize` + `list_tools` via reqwest.
  - Asserts the 35 tools are listed.
- Smoke-check with `--transport all`: both stdio + HTTP serve the same registry.

## Done means

- [ ] `--transport stdio|http|all` works as documented.
- [ ] HTTP listener responds to `/mcp` with valid MCP JSON-RPC.
- [ ] At least one HTTP-only client (Cursor recommended) confirmed against the live server.
- [ ] Wiki docs (`reference/client-config.md`, `reference/client-quirks.md`) updated; `⚠ planned` markers removed.
- [ ] CI gates still green.
- [ ] [`.repo_wiki/specification/ipc-protocol.md`](../../.repo_wiki/specification/ipc-protocol.md) unchanged (HTTP only affects the MCP↔server hop, not the IPC↔editor hop).
- [ ] Log entry in [`.repo_wiki/log.md`](../../.repo_wiki/log.md).
- [ ] This file deleted.
