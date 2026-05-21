use std::sync::Arc;
use tokio::sync::{Notify, broadcast};

use godot::builtin::Callable;
use godot::classes::editor_plugin::DockSlot;
use godot::classes::{EditorPlugin, Engine, IEditorPlugin, SceneTree, VBoxContainer};
use godot::prelude::*;

use crate::commands;
use crate::dispatcher::MainThreadDispatcher;
use crate::dock;
use crate::ipc::plugin_state::PluginState;
use crate::ipc::ws_server::IpcWebSocketServer;
use crate::logging;

const PUMP_SIGNAL: &str = "process_frame";

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
    pump_callable: Option<Callable>,
    scene_tree: Option<Gd<SceneTree>>,
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
            pump_callable: None,
            scene_tree: None,
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

        let server = IpcWebSocketServer::new(9500, state, shutdown, dispatcher.clone(), registry);
        self.broadcast_tx = Some(server.broadcast_tx());

        runtime.spawn(async move {
            let mut server = server;
            match server.run().await {
                Ok(()) => {}
                Err(e) => eprintln!("[Godot MCP] WebSocket server error: {}", e),
            }
        });

        self.runtime = Some(runtime);

        self.install_main_thread_pump(dispatcher);

        if let Some(ref tx) = self.broadcast_tx {
            let dock = dock::main_dock::create_dock(tx.clone());
            self.base_mut()
                .add_control_to_dock(DockSlot::RIGHT_UL, &dock);
            self.main_dock = Some(dock);
        }

        godot_print!("[Godot MCP] Plugin loaded!");
    }

    fn exit_tree(&mut self) {
        godot_print!("[Godot MCP] Plugin exiting tree...");

        self.uninstall_main_thread_pump();

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

        logging::drain_to_console();

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

    /// Connect a `Callable::from_fn` to `SceneTree::process_frame` so the
    /// dispatcher job queue and the cross-thread log channel are drained
    /// every frame on the main thread, in a call stack that does NOT carry
    /// an active bind borrow of this plugin.
    ///
    /// Background: executing tool closures directly from `process(&mut self)`
    /// panics with `Gd<T>::bind_mut() failed, already bound` whenever a
    /// closure invokes a Godot API (e.g. `EditorInterface::save_scene`) that
    /// re-enters the plugin via a synchronous signal callback. See
    /// gdext issue #338 ("Implicit binds in virtual calls cause borrow errors").
    fn install_main_thread_pump(&mut self, dispatcher: MainThreadDispatcher) {
        let mut tree = match self.base().get_tree_or_null() {
            Some(t) => t,
            None => {
                godot_print!(
                    "[Godot MCP] WARN: plugin not inside a SceneTree at enter_tree; pump disabled"
                );
                return;
            }
        };

        let callable = Callable::from_fn("godot_mcp_pump", move |_args| {
            dispatcher.process_pending();
            logging::drain_to_console();
            Variant::nil()
        });

        let err = tree.connect(PUMP_SIGNAL, &callable);
        if err != godot::global::Error::OK {
            godot_print!(
                "[Godot MCP] WARN: failed to connect process_frame pump: {:?}",
                err
            );
            return;
        }

        self.pump_callable = Some(callable);
        self.scene_tree = Some(tree);
    }

    fn uninstall_main_thread_pump(&mut self) {
        let (callable, mut tree) = match (self.pump_callable.take(), self.scene_tree.take()) {
            (Some(c), Some(t)) => (c, t),
            _ => return,
        };
        if tree.is_connected(PUMP_SIGNAL, &callable) {
            tree.disconnect(PUMP_SIGNAL, &callable);
        }
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
