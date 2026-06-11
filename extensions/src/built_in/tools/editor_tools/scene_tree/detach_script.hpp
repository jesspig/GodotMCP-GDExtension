// @tool register
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot_mcp {

class DetachScriptTool : public ITool {
public:
    String name() const override { return "detach_script"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Remove the script from a node";
    }
    String description() const override {
        return "Removes the script from a node via set_script(Variant()). "
               "Ctrl+Z restores the original script. "
               "If the node has no script, returns a NO_SCRIPT error (not treated as no-op).";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path";
            props["node_path"] = p;
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
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        godot::Ref<godot::Script> old_script = node->get_script();
        if (old_script.is_null()) {
            return ToolResult::err("NO_SCRIPT",
                "Node has no script attached: " + node_path);
        }
        String old_path = old_script->get_path();

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Detach Script",
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_script", godot::Variant());
            ur->add_undo_method(node, "set_script", old_script);
            ur->commit_action();
        } else {
            node->set_script(godot::Variant());
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["removed_script"] = old_path;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
