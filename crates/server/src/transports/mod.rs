use axum::{Router, Server};
use rmcp::ServiceExt;
use rmcp::transport::io::stdio;
use rmcp::transport::streamable_http_server::StreamableHttpService;

use crate::handler::GodotMcpHandler;

pub async fn run_stdio(handler: GodotMcpHandler) -> anyhow::Result<()> {
    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();
    
    let service = handler.serve((stdin, stdout)).await?;
    service.waiting().await?;
    
    Ok(())
}

pub async fn run_http(handler: GodotMcpHandler, port: u16) -> anyhow::Result<()> {
    let mcp_service = StreamableHttpService::builder()
        .service_factory(move || Ok(handler.clone()))
        .build();
    
    let app = Router::new()
        .nest_service("/mcp", mcp_service.into_router());
    
    let addr = ([127, 0, 0, 1], port).into();
    Server::bind(&addr).serve(app.into_make_service()).await?;
    
    Ok(())
}

pub async fn run_all(handler: GodotMcpHandler, port: u16) -> anyhow::Result<()> {
    let handler_clone = handler.clone();
    
    tokio::spawn(async move {
        if let Err(e) = run_stdio(handler_clone).await {
            eprintln!("stdio transport error: {}", e);
        }
    });
    
    run_http(handler, port).await?;
    
    Ok(())
}
