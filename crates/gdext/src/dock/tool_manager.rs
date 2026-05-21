use godot::classes::{CheckBox, Label, PanelContainer, VBoxContainer};
use godot::prelude::*;

use serde_json::json;

pub struct ToolManager {
    checkboxes: Vec<(String, Gd<CheckBox>)>,
    broadcast_tx: Option<tokio::sync::broadcast::Sender<String>>,
}

impl ToolManager {
    pub fn new(broadcast_tx: tokio::sync::broadcast::Sender<String>) -> Self {
        Self {
            checkboxes: Vec::new(),
            broadcast_tx: Some(broadcast_tx),
        }
    }

    pub fn create_panel(&mut self) -> Gd<PanelContainer> {
        let mut panel = PanelContainer::new_alloc();

        let mut vbox = VBoxContainer::new_alloc();

        let mut header = Label::new_alloc();
        header.set_text("Tools (4/4)");
        vbox.add_child(&header);

        let tools = vec![
            ("ping", "检测与 Godot 编辑器的连接状态", true),
            ("get_engine_version", "获取 Godot 引擎版本号", true),
            ("get_plugin_version", "获取 Godot MCP 插件版本号", true),
            ("get_server_version", "获取 godot-mcp-server 版本号", true),
        ];

        for (name, _desc, enabled) in tools {
            let mut cb = CheckBox::new_alloc();
            cb.set_text(name);
            cb.set_pressed(enabled);
            cb.set_tooltip_text(_desc);

            let name_owned = name.to_string();
            let tx = self.broadcast_tx.clone().unwrap();
            cb.connect(
                "toggled",
                &Callable::from_fn("on_toggled", move |args: &[&Variant]| {
                    let pressed = args[0].to::<bool>();
                    Self::on_tool_toggled(&name_owned, pressed, &tx);
                    Variant::nil()
                }),
            );

            vbox.add_child(&cb);
            self.checkboxes.push((name.to_string(), cb));
        }

        panel.add_child(&vbox);
        panel
    }

    fn on_tool_toggled(tool_name: &str, enabled: bool, tx: &tokio::sync::broadcast::Sender<String>) {
        godot_print!(
            "[Godot MCP] Tool '{}' {}",
            tool_name,
            if enabled { "enabled" } else { "disabled" }
        );

        let notification = godot_mcp_core::protocol::IpcNotification {
            msg_type: "notification".into(),
            event: "tool_list_updated".into(),
            data: json!({
                "tools": [
                    { "name": tool_name, "enabled": enabled }
                ]
            }),
        };
        if let Ok(json_str) = serde_json::to_string(&notification) {
            let _ = tx.send(json_str);
        }
    }
}
