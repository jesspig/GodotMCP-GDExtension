#pragma once

#include <godot_cpp/classes/control.hpp>

namespace godot_mcp {

inline godot::Control::LayoutPreset resolve_layout_preset(const String &name) {
    if (name == "full_rect") return godot::Control::PRESET_FULL_RECT;
    if (name == "center") return godot::Control::PRESET_CENTER;
    if (name == "top_left") return godot::Control::PRESET_TOP_LEFT;
    if (name == "top_right") return godot::Control::PRESET_TOP_RIGHT;
    if (name == "bottom_left") return godot::Control::PRESET_BOTTOM_LEFT;
    if (name == "bottom_right") return godot::Control::PRESET_BOTTOM_RIGHT;
    if (name == "center_left") return godot::Control::PRESET_CENTER_LEFT;
    if (name == "center_top") return godot::Control::PRESET_CENTER_TOP;
    if (name == "center_right") return godot::Control::PRESET_CENTER_RIGHT;
    if (name == "center_bottom") return godot::Control::PRESET_CENTER_BOTTOM;
    if (name == "left_wide") return godot::Control::PRESET_LEFT_WIDE;
    if (name == "top_wide") return godot::Control::PRESET_TOP_WIDE;
    if (name == "right_wide") return godot::Control::PRESET_RIGHT_WIDE;
    if (name == "bottom_wide") return godot::Control::PRESET_BOTTOM_WIDE;
    if (name == "vcenter_wide") return godot::Control::PRESET_VCENTER_WIDE;
    if (name == "hcenter_wide") return godot::Control::PRESET_HCENTER_WIDE;
    return godot::Control::PRESET_FULL_RECT;
}

} // namespace godot_mcp
