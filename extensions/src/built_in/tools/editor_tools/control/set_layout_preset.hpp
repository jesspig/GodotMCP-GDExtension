// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

static godot::Control::LayoutPreset resolve_preset_name(const String &name) {
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

class SetLayoutPresetTool : public ITool {
public:
    String name() const override { return "set_layout_preset"; }
    String category() const override { return "editor_tools/control"; }
    String brief() const override {
        return String::utf8("Set anchor preset on a Control node");
    }
    String description() const override {
        return String::utf8("Sets the anchor preset on a Control node and optionally keeps offsets. "
                            "Undo restores the previous anchor and offset values.");
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Control node path");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String::utf8("Preset name: full_rect/center/top_left/top_right/bottom_left/bottom_right/etc.");
            props["preset"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String::utf8("Keep current offset values");
            p["default"] = false;
            props["keep_offsets"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "preset");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String preset_name = args_string(ctx.args, "preset");
        bool keep_offsets = args_bool(ctx.args, "keep_offsets", false);

        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND", String::utf8("Node not found: ") + node_path);
        }
        godot::Control *control = godot::Object::cast_to<godot::Control>(node);
        if (!control) {
            return ToolResult::err("NOT_A_CONTROL", String::utf8("Node is not a Control: ") + node_path);
        }

        godot::Control::LayoutPreset preset = resolve_preset_name(preset_name);

        double old_left = control->get_anchor(godot::Side::SIDE_LEFT);
        double old_top = control->get_anchor(godot::Side::SIDE_TOP);
        double old_right = control->get_anchor(godot::Side::SIDE_RIGHT);
        double old_bottom = control->get_anchor(godot::Side::SIDE_BOTTOM);
        double old_offset_left = control->get_offset(godot::Side::SIDE_LEFT);
        double old_offset_top = control->get_offset(godot::Side::SIDE_TOP);
        double old_offset_right = control->get_offset(godot::Side::SIDE_RIGHT);
        double old_offset_bottom = control->get_offset(godot::Side::SIDE_BOTTOM);

        EditorUndoRedoManager *ur = get_undo_redo();
        ur->create_action(String::utf8("MCP: Set Layout Preset ") + preset_name,
                          godot::UndoRedo::MERGE_DISABLE, ctx.root);
        ur->add_do_method(control, "set_anchors_preset", preset, keep_offsets);
        ur->add_undo_property(control, "anchor_left", old_left);
        ur->add_undo_property(control, "anchor_top", old_top);
        ur->add_undo_property(control, "anchor_right", old_right);
        ur->add_undo_property(control, "anchor_bottom", old_bottom);
        ur->add_undo_property(control, "offset_left", old_offset_left);
        ur->add_undo_property(control, "offset_top", old_offset_top);
        ur->add_undo_property(control, "offset_right", old_offset_right);
        ur->add_undo_property(control, "offset_bottom", old_offset_bottom);
        ur->commit_action();

        Dictionary data;
        data["preset"] = preset_name;
        data["keep_offsets"] = keep_offsets;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
