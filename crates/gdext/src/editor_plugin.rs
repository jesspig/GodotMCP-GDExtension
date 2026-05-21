use std::sync::Arc;
use tokio::sync::Notify;

use godot::classes::{EditorPlugin, Engine, IEditorPlugin};
use godot::prelude::*;

use crate::ipc::plugin_state::PluginState;
use crate::ipc::ws_server::IpcWebSocketServer;

#[derive(GodotClass)]
#[class(tool, base=EditorPlugin)]
pub struct McpEditorPlugin {
    #[base]
    base: Base<EditorPlugin>,
    runtime: Option<tokio::runtime::Runtime>,
    shutdown: Option<Arc<Notify>>,
}

#[godot_api]
impl IEditorPlugin for McpEditorPlugin {
    fn init(base: Base<EditorPlugin>) -> Self {
        Self {
            base,
            runtime: None,
            shutdown: None,
        }
    }

    fn enter_tree(&mut self) {
        godot_print!("[Godot MCP] Plugin entering tree...");

        let engine_version = Self::read_engine_version();
        let plugin_version = env!("CARGO_PKG_VERSION").to_string();

        let runtime = match tokio::runtime::Builder::new_multi_thread()
            .worker_threads(2)
            .enable_all()
            .build()
        {
            Ok(rt) => rt,
            Err(e) => {
                godot_print!("[Godot MCP] Failed to build tokio runtime: {}", e);
                return;
            }
        };

        let state = Arc::new(PluginState {
            engine_version,
            plugin_version,
        });

        let shutdown = Arc::new(Notify::new());
        self.shutdown = Some(shutdown.clone());

        let server = IpcWebSocketServer::new(9500, state, shutdown);
        runtime.spawn(async move {
            let mut server = server;
            match server.run().await {
                Ok(()) => {}
                Err(e) => eprintln!("[Godot MCP] WebSocket server error: {}", e),
            }
        });

        self.runtime = Some(runtime);
        godot_print!("[Godot MCP] Plugin loaded!");
    }

    fn exit_tree(&mut self) {
        godot_print!("[Godot MCP] Plugin exiting tree...");

        if let Some(shutdown) = self.shutdown.take() {
            shutdown.notify_one();
            eprintln!("[Godot MCP] Shutdown signal sent, waiting for server to stop...");
            std::thread::sleep(std::time::Duration::from_millis(200));
        }

        self.runtime.take();

        godot_print!("[Godot MCP] Plugin unloaded!");
    }
}

impl McpEditorPlugin {
    fn read_engine_version() -> String {
        let engine = Engine::singleton();
        let version_dict = engine.get_version_info();
        let major = version_dict.get("major").unwrap().to::<i64>();
        let minor = version_dict.get("minor").unwrap().to::<i64>();
        let patch = version_dict.get("patch").unwrap().to::<i64>();
        format!("{}.{}.{}", major, minor, patch)
    }
}
