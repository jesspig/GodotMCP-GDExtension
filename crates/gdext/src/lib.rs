use godot::prelude::*;
use godot::init::InitLevel;

mod editor_plugin;
mod ipc;

struct GodotMcpExtension;

#[gdextension]
unsafe impl ExtensionLibrary for GodotMcpExtension {
    fn min_level() -> InitLevel {
        InitLevel::Editor
    }
}