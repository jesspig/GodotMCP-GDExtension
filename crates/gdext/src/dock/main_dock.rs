use godot::classes::VBoxContainer;
use godot::prelude::*;

use super::integration;
use super::settings;
use super::status_bar;
use super::tool_manager::ToolManager;

pub fn create_dock(broadcast_tx: tokio::sync::broadcast::Sender<String>) -> Gd<VBoxContainer> {
    let mut dock = VBoxContainer::new_alloc();
    dock.set_name("GodotMCP");

    let status = status_bar::create_status_bar();
    dock.add_child(&status);

    let integration_panel = integration::create_panel();
    dock.add_child(&integration_panel);

    let mut tool_mgr = ToolManager::new(broadcast_tx);
    let tool_panel = tool_mgr.create_panel();
    dock.add_child(&tool_panel);

    let settings_panel = settings::create_panel();
    dock.add_child(&settings_panel);

    dock
}
