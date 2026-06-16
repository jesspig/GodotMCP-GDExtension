
#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"
#include "control_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class SetLayoutPresetTool : public ITool {
public:
    String name() const noexcept override { return "set_layout_preset"; }
    String category() const noexcept override { return "editor_tools/control"; }
    String brief() const noexcept override {
        return String("Set anchor preset on a Control node");
    }
    String description() const override {
        return String("Sets the anchor preset on a Control node and optionally keeps offsets. "
                            "Undo restores the previous anchor and offset values.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("node_path", "string", "Control node path")
            .prop("preset", "string", "Preset name: full_rect/center/top_left/top_right/bottom_left/bottom_right/etc.")
            .prop("keep_offsets", "boolean", "Keep current offset values", false)
            .required({"node_path", "preset"})
            .build();
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        String preset_name = args_string(ctx.args, "preset");
        bool keep_offsets = args_bool(ctx.args, "keep_offsets", false);

        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        auto *control = godot::Object::cast_to<godot::Control>(node);
        if (!control) {
            return ToolResult::err("NOT_A_CONTROL", String("Node is not a Control: ") + node_path);
        }

        godot::Control::LayoutPreset preset = resolve_layout_preset(preset_name);

        double old_left = control->get_anchor(godot::Side::SIDE_LEFT);
        double old_top = control->get_anchor(godot::Side::SIDE_TOP);
        double old_right = control->get_anchor(godot::Side::SIDE_RIGHT);
        double old_bottom = control->get_anchor(godot::Side::SIDE_BOTTOM);
        double old_offset_left = control->get_offset(godot::Side::SIDE_LEFT);
        double old_offset_top = control->get_offset(godot::Side::SIDE_TOP);
        double old_offset_right = control->get_offset(godot::Side::SIDE_RIGHT);
        double old_offset_bottom = control->get_offset(godot::Side::SIDE_BOTTOM);

        auto *ur = begin_undo_action("MCP: Set Layout Preset " + preset_name);
        if (!ur) {
            control->set_anchors_preset(preset, keep_offsets);
            mark_scene_dirty();
        } else {
            ur->add_do_method(control, "set_anchors_preset", preset, keep_offsets);
            ur->add_undo_property(control, "anchor_left", old_left);
            ur->add_undo_property(control, "anchor_top", old_top);
            ur->add_undo_property(control, "anchor_right", old_right);
            ur->add_undo_property(control, "anchor_bottom", old_bottom);
            ur->add_undo_property(control, "offset_left", old_offset_left);
            ur->add_undo_property(control, "offset_top", old_offset_top);
            ur->add_undo_property(control, "offset_right", old_offset_right);
            ur->add_undo_property(control, "offset_bottom", old_offset_bottom);
            commit_undo_action(ur);
        }

        Dictionary data;
        data["preset"] = preset_name;
        data["keep_offsets"] = keep_offsets;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

