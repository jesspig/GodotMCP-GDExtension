
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

static godot::Control::LayoutPreset parse_layout_preset(const String &name) {
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

class CreateControlTool : public ITool {
public:
    String name() const override { return "create_control"; }
    String category() const override { return "editor_tools/control"; }
    String brief() const override {
        return "Create a Control subclass node (Button, Label, Panel, etc.)";
    }
    String description() const override {
        return "Creates a Control-subclass node under the specified parent. "
               "Supports layout presets (full_rect, center, top_left, etc.) and optional size. "
               "Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path";
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Control class name (e.g. Button, Label, Panel, ColorRect, TextureRect)";
            props["class_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name (empty = use class name)";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Layout preset: full_rect/center/top_left/top_right/bottom_left/bottom_right";
            props["layout_preset"] = p;
        }
        {
            Dictionary p;
            p["type"] = "object";
            p["description"] = "Optional size {width, height}";
            props["size"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("parent_path", "class_name");
        return s;
    }
    bool needs_scene() const override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path");
        String class_name = args_string(ctx.args, "class_name");
        String node_name = args_string(ctx.args, "node_name");
        String layout_preset = args_string(ctx.args, "layout_preset");

        if (class_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", "class_name is required");
        }
        if (!godot::ClassDB::class_exists(class_name)) {
            return ToolResult::err("UNKNOWN_CLASS", "Unknown Godot class: " + class_name);
        }
        if (!godot::ClassDB::is_parent_class(class_name, "Control")) {
            return ToolResult::err("NOT_A_CONTROL", "Class is not a Control subclass: " + class_name);
        }

        Node *parent = resolve_node(ctx.root, parent_path);
        if (!parent) {
            return ToolResult::err("NODE_NOT_FOUND", "Parent node not found: " + parent_path);
        }

        if (node_name.is_empty()) {
            node_name = class_name;
        }

        Node *child = godot::Object::cast_to<godot::Node>(godot::ClassDB::instantiate(class_name));
        if (!child) {
            return ToolResult::err("CREATE_FAILED", "Failed to create node of type: " + class_name);
        }
        child->set_name(node_name);

        if (parent->has_node(String("./") + node_name)) {
            memdelete(child);
            return ToolResult::err("NAME_CONFLICT", "A node with the same name already exists: " + node_name);
        }

        godot::Control *control = godot::Object::cast_to<godot::Control>(child);
        if (!control) {
            memdelete(child);
            return ToolResult::err("CAST_FAILED", "Failed to cast node to Control");
        }

        if (!layout_preset.is_empty()) {
            control->set_anchors_preset(parse_layout_preset(layout_preset));
        }

        Dictionary size_dict;
        if (ctx.args.has("size") && ctx.args["size"].get_type() == Variant::DICTIONARY) {
            size_dict = ctx.args["size"];
        }
        if (!size_dict.is_empty() && size_dict.has("width") && size_dict.has("height")) {
            control->set_size(godot::Vector2(
                (real_t)args_float(size_dict, "width", 0.0),
                (real_t)args_float(size_dict, "height", 0.0)
            ));
        }

        EditorUndoRedoManager *ur = get_undo_redo();
        if (!ur) {
            parent->add_child(child, true, godot::Node::INTERNAL_MODE_DISABLED);
            child->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            ur->create_action(String("MCP: Create Control ") + class_name,
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(parent, "add_child", child, true, (int64_t)Node::INTERNAL_MODE_DISABLED);
            ur->add_do_method(child, "set_owner", ctx.root);
            ur->add_undo_method(parent, "remove_child", child);
            ur->add_undo_method(child, "set_owner", Variant());
            ur->add_do_reference(child);
            ur->commit_action();
        }

        EditorInterface *ei = EditorInterface::get_singleton();
        if (ei) {
            EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(child);
            }
        }

        Dictionary data;
        data["node_name"] = child->get_name();
        data["node_path"] = relative_path(ctx.root, child);
        data["class_name"] = class_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
