
#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot_mcp {

inline godot::Object *find_debugger() {
    auto *ei = godot::EditorInterface::get_singleton();
    if (!ei) return nullptr;
    auto *base = ei->get_base_control();
    if (!base) return nullptr;
    godot::Array nodes = base->find_children("*", "EditorDebuggerNode", true, false);
    if (nodes.size() == 0) return nullptr;
    return godot::Object::cast_to<godot::Node>(nodes[0]);
}

inline godot::RichTextLabel *find_console_rtl() {
    auto *ei = godot::EditorInterface::get_singleton();
    if (!ei) return nullptr;
    auto *base = ei->get_base_control();
    if (!base) return nullptr;
    godot::Array logs = base->find_children("*", "EditorLog", true, false);
    if (logs.size() == 0) return nullptr;
    auto *log = godot::Object::cast_to<godot::Node>(logs[0]);
    if (!log) return nullptr;
    godot::Array rtls = log->find_children("*", "RichTextLabel", true, false);
    if (rtls.size() == 0) return nullptr;
    return godot::Object::cast_to<godot::RichTextLabel>(rtls[0]);
}

} // namespace godot_mcp
