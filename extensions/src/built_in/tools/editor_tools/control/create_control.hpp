
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/undo_helpers.hpp"
#include "built_in/cmd_utils/args_get_typed.hpp"
#include "built_in/tools/editor_tools/scene_tree/scene_tree_utils.hpp"
#include "control_utils.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class CreateControlTool : public ITool {
public:
    String name() const noexcept override { return "create_control"; }
    String category() const noexcept override { return "editor_tools/control"; }
    String brief() const noexcept override {
        return "Create a Control subclass node (Button, Label, Panel, etc.)";
    }
    String description() const override {
        return "Creates a Control-subclass node under the specified parent. "
               "Supports layout presets (full_rect, center, top_left, etc.) and optional size. "
               "Uses EditorUndoRedoManager for undo support.";
    }
    Dictionary build_input_schema() const override {
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

        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
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

        auto *control = godot::Object::cast_to<godot::Control>(child);
        if (!control) {
            memdelete(child);
            return ToolResult::err("CAST_FAILED", "Failed to cast node to Control");
        }

        if (!layout_preset.is_empty()) {
            control->set_anchors_preset(resolve_layout_preset(layout_preset));
        }

        Dictionary size_dict = args_get_typed<Dictionary>(ctx.args, "size", Dictionary());
        if (!size_dict.is_empty() && size_dict.has("width") && size_dict.has("height")) {
            control->set_size(godot::Vector2(
                static_cast<real_t>(args_float(size_dict, "width", 0.0)),
                static_cast<real_t>(args_float(size_dict, "height", 0.0))
            ));
        }

        auto *ur = begin_undo_action("MCP: Create Control " + class_name);
        if (!ur) {
            parent->add_child(child, true, godot::Node::INTERNAL_MODE_DISABLED);
            child->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            commit_add_child_undo(ur, "MCP: Create Control " + class_name, parent, child, ctx.root);
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
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
