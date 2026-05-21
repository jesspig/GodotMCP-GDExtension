use godot::init::InitLevel;
use godot::prelude::*;

mod editor_plugin;
mod ipc;

struct GodotMcpExtension;

#[gdextension]
unsafe impl ExtensionLibrary for GodotMcpExtension {
    fn min_level() -> InitLevel {
        InitLevel::Editor
    }
}
