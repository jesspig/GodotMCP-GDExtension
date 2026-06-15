#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot_mcp {

class PasteNodeTool : public ITool {
public:
    String name() const override { return "paste_node"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Paste a node from the clipboard";
    }
    String description() const override {
        return "Instantiates a node from the internal PackedScene clipboard. "
               "mode: child (as child of target_path, default), "
               "sibling (as sibling of target_path, inserted after it), "
               "replacement (replaces the target_path node). "
               "Returns an error if the clipboard is empty. Ctrl+Z restores a replaced node.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Target node path (required for mode=sibling/replacement)";
            props["target_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("child | sibling | replacement");
            p["default"] = "child";
            p["enum"] = Array::make("child", "sibling", "replacement");
            props["mode"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "New node name (empty = keep clipboard name)";
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
        String target_path = args_string(ctx.args, "target_path", "");
        String mode = args_string(ctx.args, "mode", "child");
        String new_name = args_string(ctx.args, "new_name", "");

        godot::Ref<godot::PackedScene> clipboard = scene_tree_utils::get_clipboard();
        if (clipboard.is_null()) {
            return ToolResult::err("EMPTY_CLIPBOARD",
                "Clipboard is empty, use copy_node or cut_node first");
        }

        Node *target = nullptr;
        Node *parent = nullptr;
        int64_t target_index = -1;
        if (!target_path.is_empty()) {
            target = resolve_node(ctx.root, target_path);
            if (!target) {
                return ToolResult::err("TARGET_NOT_FOUND",
                    "Target node not found: " + target_path);
            }
        }
        if (mode == "child") {
            parent = target ? target : ctx.root;
        } else if (mode == "sibling") {
            if (!target) {
                return ToolResult::err("MISSING_TARGET",
                    "sibling mode requires target_path");
            }
            parent = target->get_parent();
            if (!parent) {
                return ToolResult::err("ORPHAN_TARGET",
                    "Target node has no parent");
            }
            target_index = target->get_index() + 1;
        } else if (mode == "replacement") {
            if (!target) {
                return ToolResult::err("MISSING_TARGET",
                    "replacement mode requires target_path");
            }
            parent = target->get_parent();
            if (!parent) {
                return ToolResult::err("ORPHAN_TARGET",
                    "Target node has no parent");
            }
            target_index = target->get_index();
        } else {
            return ToolResult::err("BAD_MODE",
                "mode must be child | sibling | replacement");
        }

        Node *inst = scene_tree_utils::instantiate_subtree(clipboard);
        if (!inst) {
            return ToolResult::err("INSTANTIATE_FAILED",
                "Failed to instantiate node from clipboard");
        }
        if (!new_name.is_empty()) {
            inst->set_name(new_name);
        }
        // owner wired post-add_child (use a do_method so it captures in undo)

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Paste " + inst->get_name(),
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            if (mode == "replacement") {
                ur->add_do_method(target, "replace_by", inst, true);
                ur->add_undo_method(inst, "replace_by", target, true);
                ur->add_do_reference(inst);
                ur->add_undo_reference(inst);
                ur->add_do_reference(target);
                ur->add_undo_reference(target);
            } else {
                ur->add_do_method(parent, "add_child", inst, true,
                                  static_cast<int64_t>(godot::Node::INTERNAL_MODE_DISABLED));
                if (target_index >= 0) {
                    ur->add_do_method(parent, "move_child", inst, target_index);
                }
                ur->add_undo_method(parent, "remove_child", inst);
                ur->add_do_reference(inst);
                ur->add_undo_reference(inst);
            }
            // Set owner as part of undo action
            {
                godot::UndoRedo *undo_redo = ur->get_history_undo_redo(
                    ur->get_object_history_id(ctx.root));
                auto assign_callable = callable_mp_static(scene_tree_utils::assign_owner_recursive);
                undo_redo->add_do_method(assign_callable.bind(inst, ctx.root));
                undo_redo->add_undo_method(assign_callable.bind(inst, ctx.root));
            }

            ur->commit_action();

            // Also set owner outside undo for immediate consistency
            scene_tree_utils::assign_owner_recursive(inst, ctx.root);
        } else {
            if (mode == "replacement") {
                target->replace_by(inst, true);
            } else {
                parent->add_child(inst, true, godot::Node::INTERNAL_MODE_DISABLED);
                if (target_index >= 0) {
                    parent->move_child(inst, target_index);
                }
            }
            scene_tree_utils::assign_owner_recursive(inst, ctx.root);
        }

        // select new node
        godot::EditorInterface *ei = godot::EditorInterface::get_singleton();
        if (ei) {
            godot::EditorSelection *sel = ei->get_selection();
            if (sel) {
                sel->clear();
                sel->add_node(inst);
            }
        }

        // Compute path manually �?inst may not have a valid scene path yet
        // (undo commit may add it asynchronously relative to scene root)
        String new_node_path;
        if (parent == ctx.root || !parent) {
            new_node_path = inst->get_name();
        } else {
            String parent_rel = relative_path(ctx.root, parent);
            new_node_path = parent_rel.is_empty() ? String(inst->get_name()) : parent_rel + "/" + inst->get_name();
        }
        Dictionary data;
        data["new_node"] = new_node_path;
        data["type"] = inst->get_class();
        data["mode"] = mode;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

