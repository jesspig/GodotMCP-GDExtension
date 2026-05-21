use godot::classes::{Button, Label, PanelContainer, VBoxContainer};
use godot::prelude::*;

pub fn create_panel() -> Gd<PanelContainer> {
    let mut panel = PanelContainer::new_alloc();

    let mut vbox = VBoxContainer::new_alloc();

    let mut header = Label::new_alloc();
    header.set_text("Client Integration");
    vbox.add_child(&header);

    let clients = [
        "Claude Code",
        "Codex",
        "Gemini CLI",
        "OpenCode",
        "Cursor",
        "GitHub Copilot",
        "Qwen Code",
        "Trae",
        "Trae CN",
        "Qoder",
        "Antigravity",
        "CodeBuddy",
    ];

    for client in clients {
        let mut row = Button::new_alloc();
        row.set_text(client);
        row.set_tooltip_text("Phase 3 实现");
        row.set_disabled(true);
        vbox.add_child(&row);
    }

    panel.add_child(&vbox);
    panel
}
