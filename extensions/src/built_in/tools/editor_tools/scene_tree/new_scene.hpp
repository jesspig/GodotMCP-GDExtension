
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class NewSceneTool : public ITool {
public:
    String name() const noexcept override { return "new_scene"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Create a new scene with a specified root node type and name";
    }
    String description() const override {
        return "Creates a new blank scene in the editor. The root node type is specified by class_name "
               "(e.g. \"Node2D\", \"Node3D\", \"Control\", \"Node\"). "
               "Optionally specify the root name; the scene is held in memory and can be saved via save_scene.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Root node type (Godot class name, e.g. Node2D, Node3D, Control, Node)";
            props["root_type"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Root node name (default = type name)";
            props["root_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("root_type");
        return s;
    }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String root_type = args_string(ctx.args, "root_type");
        String root_name = args_string(ctx.args, "root_name");
        if (root_type.is_empty()) {
            return ToolResult::err("MISSING_ARG", "root_type cannot be empty");
        }
        if (!godot::ClassDB::class_exists(root_type)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown Godot class: " + root_type);
        }
        if (root_name.is_empty()) {
            root_name = root_type;
        }

        Node *new_root = scene_tree_utils::create_node(root_type, root_name);
        if (!new_root) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + root_type);
        }

        auto *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            memdelete(new_root);
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }

        // add_root_node() requires the current scene tab to have no root.
        // close_scene() discards the current tab and creates a new empty one
        // (root = nullptr), matching the editor's new_scene() flow.
        Node *current_root = ei->get_edited_scene_root();
        if (current_root) {
            ei->close_scene();
        }

        ei->add_root_node(new_root);

        auto *sel = ei->get_selection();
        if (sel) {
            sel->clear();
            sel->add_node(new_root);
        }

        Dictionary data;
        data["root_type"] = root_type;
        data["root_name"] = new_root->get_name();
        data["path"] = ".";
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
