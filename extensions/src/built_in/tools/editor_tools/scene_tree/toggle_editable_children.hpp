#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class ToggleEditableChildrenTool : public ITool {
public:
    String name() const noexcept override { return "toggle_editable_children"; }
    String category() const noexcept override { return "editor_tools/scene_tree"; }
    String brief() const noexcept override {
        return "Toggle Editable Children on an instanced scene";
    }
    String description() const override {
        return "Toggles the Editable Children state on a scene instance node (a node with a scene_file_path). "
               "enable=true allows editing children (changes do not sync back to the original scene); "
               "false locks it to a read-only instance view. "
               "Non-instance nodes return an error. All changes are undoable.";
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Scene instance node path";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "true = enable editable children, false = disable";
            props["enable"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String node_path = args_string(ctx.args, "node_path");
        // default: toggle (use inverse of current state)
        Node *node = nullptr;
        if (auto err = scene_tree_utils::resolve_node_or_error(ctx.root, node_path, node)) {
            return ToolResult::err("NODE_NOT_FOUND", err->get("message", ""));
        }
        if (node->get_scene_file_path().is_empty()) {
            return ToolResult::err("NOT_AN_INSTANCE",
                "Node is not a scene instance (no scene_file_path)");
        }
        // set_editable_instance(child, flag) is called on the PARENT, marking
        // `child`'s scene-internal subtree editable. Calling node->set_editable_instance(node, ...)
        // (node passed as its own child) is a silent no-op �?the previous code
        // did exactly that, so the toggle never took effect.
        Node *parent = node->get_parent();
        if (!parent) {
            return ToolResult::err("ORPHAN_NODE",
                "Node has no parent; editable children cannot be toggled");
        }
        bool current = parent->is_editable_instance(node);
        bool enable;
        if (ctx.args.has("enable")) {
            enable = static_cast<bool>(ctx.args["enable"]);
        } else {
            enable = !current;
        }
        if (current == enable) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["editable_children"] = enable;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        auto *ur = begin_undo_action("MCP: Toggle Editable Children");
        if (ur) {
            ur->add_do_method(parent, "set_editable_instance", node, enable);
            ur->add_undo_method(parent, "set_editable_instance", node, current);
            commit_undo_action(ur);
        } else {
            parent->set_editable_instance(node, enable);
        }
        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["editable_children"] = enable;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp

