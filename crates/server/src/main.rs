use clap::Parser;
use rmcp::ServiceExt;

mod bridge;
mod handler;

#[derive(Parser, Debug)]
#[command(name = "godot-mcp-server")]
#[command(about = "MCP server for Godot Engine integration")]
struct Cli {
    #[arg(long, default_value_t = 9500)]
    godot_port: u16,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();
    let handler = handler::GodotMcpHandler::new(cli.godot_port);

    eprintln!("[MCP Server] Starting on stdio...");
    let service = handler
        .serve((tokio::io::stdin(), tokio::io::stdout()))
        .await?;
    service.waiting().await?;

    Ok(())
}