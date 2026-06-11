// =====================================================================
// logging.hpp — Thin wrapper around UtilityFunctions::print*.
//
// The original Rust implementation used an MPSC channel because worker
// threads couldn't safely call godot_print!. The C++ rewrite executes
// every command on the main thread (inside EditorPlugin::_process()),
// so we can call into the engine directly.
// =====================================================================

#pragma once

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot_mcp {

inline void log_info(const godot::String &scope, const godot::String &message) {
    godot::UtilityFunctions::print("[godot-mcp][", scope, "] ", message);
}

inline void log_warn(const godot::String &scope, const godot::String &message) {
    godot::UtilityFunctions::push_warning("[godot-mcp][", scope, "] ", message);
}

inline void log_error(const godot::String &scope, const godot::String &message) {
    godot::UtilityFunctions::push_error("[godot-mcp][", scope, "] ", message);
}

}  // namespace godot_mcp
