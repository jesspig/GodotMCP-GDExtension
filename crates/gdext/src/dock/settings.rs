use godot::classes::{Button, HBoxContainer, Label, LineEdit, PanelContainer, VBoxContainer};
use godot::prelude::*;

pub fn create_panel() -> Gd<PanelContainer> {
    let mut panel = PanelContainer::new_alloc();

    let mut vbox = VBoxContainer::new_alloc();

    let mut header = Label::new_alloc();
    header.set_text("Advanced Settings");
    vbox.add_child(&header);

    let mut port_row = HBoxContainer::new_alloc();

    let mut ws_label = Label::new_alloc();
    ws_label.set_text("WS Port:");
    port_row.add_child(&ws_label);

    let mut ws_port = LineEdit::new_alloc();
    ws_port.set_text("9500");
    ws_port.set_editable(false);
    port_row.add_child(&ws_port);

    let mut http_label = Label::new_alloc();
    http_label.set_text("HTTP Port:");
    port_row.add_child(&http_label);

    let mut http_port = LineEdit::new_alloc();
    http_port.set_text("8900");
    http_port.set_editable(false);
    port_row.add_child(&http_port);

    vbox.add_child(&port_row);

    let mut action_row = HBoxContainer::new_alloc();

    let mut restart_btn = Button::new_alloc();
    restart_btn.set_text("Restart");
    restart_btn.set_disabled(true);
    restart_btn.set_tooltip_text("Phase 3 实现");
    action_row.add_child(&restart_btn);

    let mut logs_btn = Button::new_alloc();
    logs_btn.set_text("Open Logs");
    logs_btn.set_disabled(true);
    logs_btn.set_tooltip_text("Phase 3 实现");
    action_row.add_child(&logs_btn);

    vbox.add_child(&action_row);

    panel.add_child(&vbox);
    panel
}
