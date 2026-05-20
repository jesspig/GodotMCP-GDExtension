use clap::Parser;
use std::io::{self, Read, Write};
use serde_json::{json, Value};

#[derive(Parser, Debug)]
#[command(name = "godot-mcp-server")]
#[command(about = "MCP server for Godot Engine integration")]
#[command(version = "0.1.0")]
struct Cli {
    #[arg(long, default_value = "stdio")]
    transport: String,
    
    #[arg(long, default_value_t = 8900)]
    port: u16,
    
    #[arg(long, default_value_t = 9500)]
    godot_port: u16,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let _cli = Cli::parse();
    
    eprintln!("Godot MCP Server starting...");
    
    // Simple stdio server that responds to ping
    let mut input = String::new();
    loop {
        input.clear();
        io::stdin().read_line(&mut input)?;
        
        if input.trim().is_empty() {
            continue;
        }
        
        // Try to parse as JSON
        match serde_json::from_str::<Value>(&input) {
            Ok(msg) => {
                eprintln!("Received: {:?}", msg);
                
                // Check if it's an initialize request
                let response = if msg.get("method") == Some(&json!("initialize")) {
                    json!({
                        "jsonrpc": "2.0",
                        "id": msg.get("id"),
                        "result": {
                            "protocolVersion": "2024-11-05",
                            "capabilities": {
                                "tools": {}
                            },
                            "serverInfo": {
                                "name": "godot-mcp-server",
                                "version": "0.1.0"
                            }
                        }
                    })
                } else if msg.get("method") == Some(&json!("tools/list")) {
                    json!({
                        "jsonrpc": "2.0",
                        "id": msg.get("id"),
                        "result": {
                            "tools": [
                                {
                                    "name": "ping",
                                    "description": "Ping the server",
                                    "inputSchema": {
                                        "type": "object",
                                        "properties": {},
                                        "required": []
                                    }
                                }
                            ]
                        }
                    })
                } else if msg.get("method") == Some(&json!("tools/call")) {
                    let result = json!({
                        "content": [
                            {
                                "type": "text",
                                "text": "pong"
                            }
                        ]
                    });
                    json!({
                        "jsonrpc": "2.0",
                        "id": msg.get("id"),
                        "result": result
                    })
                } else {
                    json!({
                        "jsonrpc": "2.0",
                        "id": msg.get("id"),
                        "result": {}
                    })
                };
                
                serde_json::to_writer(io::stdout(), &response)?;
                io::stdout().write_all(b"\n")?;
                io::stdout().flush()?;
            }
            Err(e) => {
                eprintln!("Error parsing JSON: {}", e);
            }
        }
    }
}
