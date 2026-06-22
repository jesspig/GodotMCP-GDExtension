
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "built_in/cmd_utils/memdelete_guard.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class AddNodeTool : public ITool {
public:
    String name() const noexcept override { return "add_node"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Add a new node to the scene";
    }
    String description() const override {
        return "Creates a new node under the specified parent. "
               "class_name is the Godot class name (e.g. Node2D, Node3D, Button). "
               "name defaults to the type name if left empty. "
               "index=-1 appends at the end. "
               "All changes go through EditorUndoRedoManager and can be undone with Ctrl+Z.";
    }
    String category_description() const noexcept override {
        return "Editor operation tools: scene tree CRUD, clipboard, script, workspace switching, console, debugger, performance monitors, etc.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Parent node path (empty = scene root)";
            p["x-mcp-header"] = true;
            props["parent_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node type to create (Godot class name)";
            props["class_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node name (empty = default type name)";
            props["node_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "integer";
            p["description"] = "Insert position (-1 = append at end)";
            p["default"] = static_cast<int64_t>(-1);
            props["index"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("class_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String parent_path = args_string(ctx.args, "parent_path", "");
        String class_name = args_string(ctx.args, "class_name");
        String node_name = args_string(ctx.args, "node_name");
        if (node_name.is_empty()) {
            node_name = args_string(ctx.args, "name");
        }
        int64_t index = args_int(ctx.args, "index", -1);

        if (class_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", "class_name cannot be empty");
        }
        if (!godot::ClassDB::class_exists(class_name)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown Godot class: " + class_name);
        }
        Node *parent = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, parent_path, parent)) {
            godot::Dictionary nested = err->get("error", godot::Dictionary());
            return ToolResult::err("NODE_NOT_FOUND", nested.get("message", parent_path));
        }
        if (node_name.is_empty()) {
            node_name = class_name;
        }

        Node *child = scene_tree_utils::create_node(class_name, node_name);
        if (!child) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + class_name);
        }
        MemdeleteGuard<Node> guard(child);

        if (parent->has_node(String("./") + node_name)) {
            return ToolResult::err("NAME_CONFLICT",
                "A node with the same name already exists: " + node_name);
        }

        auto *ur = get_undo_redo();
        if (!ur) {
            parent->add_child(child, true, godot::Node::INTERNAL_MODE_DISABLED);
            child->set_owner(ctx.root);
            mark_scene_dirty();
        } else {
            scene_tree_utils::do_add_child(ur, parent, child, ctx.root, index,
                "MCP: Add " + class_name);
        }
        guard.dismiss();

        // Select the new node
        auto *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            auto *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(child);
            }
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, child);
        data["type"] = child->get_class();
        data["name"] = child->get_name();
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
