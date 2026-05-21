use std::sync::Arc;
use tokio::sync::{Notify, broadcast};

use godot::classes::editor_plugin::DockSlot;
use godot::classes::{EditorPlugin, Engine, IEditorPlugin, VBoxContainer};
use godot::prelude::*;

use crate::commands;
use crate::dispatcher::MainThreadDispatcher;
use crate::dock;
use crate::ipc::plugin_state::PluginState;
use crate::ipc::ws_server::IpcWebSocketServer;

#[derive(GodotClass)]
#[class(tool, base=EditorPlugin)]
pub struct McpEditorPlugin {
    #[base]
    base: Base<EditorPlugin>,
    runtime: Option<tokio::runtime::Runtime>,
    shutdown: Option<Arc<Notify>>,
    dispatcher: Option<MainThreadDispatcher>,
    broadcast_tx: Option<broadcast::Sender<String>>,
    main_dock: Option<Gd<VBoxContainer>>,
}

#[godot_api]
impl IEditorPlugin for McpEditorPlugin {
    fn init(base: Base<EditorPlugin>) -> Self {
        Self {
            base,
            runtime: None,
            shutdown: None,
            dispatcher: None,
            broadcast_tx: None,
            main_dock: None,
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

        let dispatcher = MainThreadDispatcher::new();
        self.dispatcher = Some(dispatcher.clone());

        let shutdown = Arc::new(Notify::new());
        self.shutdown = Some(shutdown.clone());

        let registry = commands::create_registry();

        let server = IpcWebSocketServer::new(9500, state, shutdown, dispatcher, registry);
        self.broadcast_tx = Some(server.broadcast_tx());

        runtime.spawn(async move {
            let mut server = server;
            match server.run().await {
                Ok(()) => {}
                Err(e) => eprintln!("[Godot MCP] WebSocket server error: {}", e),
            }
        });

        self.runtime = Some(runtime);

        if let Some(ref tx) = self.broadcast_tx {
            let dock = dock::main_dock::create_dock(tx.clone());
            self.base_mut()
                .add_control_to_dock(DockSlot::RIGHT_UL, &dock);
            self.main_dock = Some(dock);
        }

        godot_print!("[Godot MCP] Plugin loaded!");
    }

    fn process(&mut self, _delta: f64) {
        if let Some(ref dispatcher) = self.dispatcher {
            dispatcher.process_pending();
        }
    }

    fn exit_tree(&mut self) {
        godot_print!("[Godot MCP] Plugin exiting tree...");

        if let Some(dock) = self.main_dock.take() {
            self.base_mut().remove_control_from_docks(&dock);
            dock.free();
        }

        if let Some(shutdown) = self.shutdown.take() {
            shutdown.notify_one();
            eprintln!("[Godot MCP] Shutdown signal sent, waiting for server to stop...");
            std::thread::sleep(std::time::Duration::from_millis(200));
        }

        self.dispatcher = None;
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

    #[allow(dead_code)]
    pub fn broadcast_tool_list_updated(&self, tools: serde_json::Value) {
        if let Some(ref tx) = self.broadcast_tx {
            let notification = godot_mcp_core::protocol::IpcNotification {
                msg_type: "notification".into(),
                event: "tool_list_updated".into(),
                data: tools,
            };
            if let Ok(json) = serde_json::to_string(&notification) {
                let _ = tx.send(json);
            }
        }
    }
}
