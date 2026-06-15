#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot_mcp {

class GroupSelectedNodesTool : public ITool {
public:
    String name() const noexcept override { return "group_selected_nodes"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Group multiple selected nodes under a new parent node";
    }
    String description() const override {
        return "Equivalent to the editor \"Group Selected\" operation. "
               "Reads all top-level nodes from EditorSelection, creates a new node (of new_class type) "
               "under their common parent, then moves all selected nodes into it. "
               "Requires at least 2 selected nodes sharing the same parent. "
               "All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "New parent node type (Godot class name)";
            p["default"] = "Node";
            props["new_class"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "New parent node name (empty = type name)";
            props["new_name"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String new_class = args_string(ctx.args, "new_class", "Node");
        String new_name = args_string(ctx.args, "new_name", "");
        if (!godot::ClassDB::class_exists(new_class)) {
            return ToolResult::err("UNKNOWN_CLASS",
                "Unknown Godot class: " + new_class);
        }
        if (new_name.is_empty()) new_name = new_class;

        auto *ei = godot::EditorInterface::get_singleton();
        if (!ei) {
            return ToolResult::err("NO_EDITOR", "EditorInterface not available");
        }
        auto *sel = ei->get_selection();
        if (!sel) {
            return ToolResult::err("NO_SELECTION", "Failed to get EditorSelection");
        }
        godot::TypedArray<godot::Node> top = sel->get_top_selected_nodes();
        if (top.size() < 2) {
            return ToolResult::err("INSUFFICIENT_SELECTION",
                "Need at least 2 top-level selected nodes (current: " +
                String::num_int64(static_cast<int64_t>(top.size())) + String(")"));
        }
        // Validate all share the same parent
        Node *common_parent = nullptr;
        godot::TypedArray<int64_t> indices;
        for (int64_t i = 0; i < top.size(); i++) {
            Node *n = godot::Object::cast_to<godot::Node>(top[i]);
            if (!n) continue;
            Node *p = n->get_parent();
            if (!p) {
            return ToolResult::err("ORPHAN_SELECTED",
                "Selected node " + n->get_name() +
                " has no parent");
            }
            if (common_parent == nullptr) {
                common_parent = p;
            } else if (p != common_parent) {
                return ToolResult::err("DIFFERENT_PARENTS",
                    "Selected nodes must share the same parent");
            }
        }
        if (!common_parent) {
            return ToolResult::err("NO_VALID_SELECTION",
                "No valid selected nodes");
        }
        if (common_parent->has_node(String("./") + new_name)) {
            return ToolResult::err("NAME_CONFLICT",
                "A node with the same name already exists: " + new_name);
        }

        Node *wrapper = scene_tree_utils::create_node(new_class, new_name);
        if (!wrapper) {
            return ToolResult::err("CREATE_FAILED",
                "Failed to create node of type: " + new_class);
        }
        scene_tree_utils::assign_owner_recursive(wrapper, ctx.root);

        // capture indices and nodes for undo
        godot::Array nodes_arr;
        godot::Array indices_arr;
        for (int64_t i = 0; i < top.size(); i++) {
            Node *n = godot::Object::cast_to<godot::Node>(top[i]);
            if (!n) continue;
            nodes_arr.append(n);
            indices_arr.append(static_cast<int64_t>(n->get_index()));
        }

        auto *ur = begin_undo_action("MCP: Group Selected Nodes");
        if (ur) {
            int64_t min_idx = static_cast<int64_t>(indices_arr[0]);
            for (int i = 1; i < indices_arr.size(); i++) {
                if (static_cast<int64_t>(indices_arr[i]) < min_idx) min_idx = static_cast<int64_t>(indices_arr[i]);
            }
            ur->add_do_method(common_parent, "add_child", wrapper, true,
                              static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
            ur->add_do_method(common_parent, "move_child", wrapper, min_idx);

            // do: move each selected into wrapper
            for (int64_t i = 0; i < nodes_arr.size(); i++) {
                Node *n = godot::Object::cast_to<godot::Node>(nodes_arr[i]);
                int64_t old_idx = static_cast<int64_t>(indices_arr[i]);
                ur->add_do_method(common_parent, "remove_child", n);
                ur->add_do_method(wrapper, "add_child", n, true,
                                  static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
                ur->add_do_reference(n);
                ur->add_undo_reference(n);
                // undo: move back
                ur->add_undo_method(wrapper, "remove_child", n);
                ur->add_undo_method(common_parent, "add_child", n, true,
                                    static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
                ur->add_undo_method(common_parent, "move_child", n, old_idx);
            }

            ur->add_do_reference(wrapper);
            ur->add_undo_reference(wrapper);
            commit_undo_action(ur);
        } else {
            int64_t min_idx = static_cast<int64_t>(indices_arr[0]);
            for (int i = 1; i < indices_arr.size(); i++) {
                if (static_cast<int64_t>(indices_arr[i]) < min_idx) min_idx = static_cast<int64_t>(indices_arr[i]);
            }
            common_parent->add_child(wrapper, true, godot::Node::INTERNAL_MODE_DISABLED);
            common_parent->move_child(wrapper, min_idx);
            for (int64_t i = 0; i < nodes_arr.size(); i++) {
                Node *n = godot::Object::cast_to<godot::Node>(nodes_arr[i]);
                common_parent->remove_child(n);
                wrapper->add_child(n, true, godot::Node::INTERNAL_MODE_DISABLED);
            }
        }

        Dictionary data;
        data["wrapper"] = relative_path(ctx.root, wrapper);
        data["wrapper_type"] = wrapper->get_class();
        data["grouped"] = static_cast<int64_t>(nodes_arr.size());
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

