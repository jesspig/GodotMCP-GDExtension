use godot::classes::control::SizeFlags;
use godot::classes::{Button, ColorRect, Control, HBoxContainer, Label};
use godot::prelude::*;

pub fn create_status_bar() -> Gd<HBoxContainer> {
    let mut container = HBoxContainer::new_alloc();

    let mut indicator = ColorRect::new_alloc();
    indicator.set_custom_minimum_size(Vector2::new(12.0, 12.0));
    indicator.set_color(Color::from_rgb(0.0, 0.8, 0.0));
    container.add_child(&indicator);

    let mut label = Label::new_alloc();
    label.set_text("Status: Running");
    container.add_child(&label);

    let mut spacer = Control::new_alloc();
    spacer.set_h_size_flags(SizeFlags::EXPAND);
    container.add_child(&spacer);

    let mut button = Button::new_alloc();
    button.set_text("Stop");
    container.add_child(&button);

    container
}
