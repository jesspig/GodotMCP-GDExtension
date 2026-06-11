
#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"
#include "scene_tree_utils.hpp"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>

namespace godot_mcp {

class SetUniqueNameTool : public ITool {
public:
    String name() const override { return "set_unique_name"; }
    String category() const override { return "editor_tools/scene_tree"; }
    String brief() const override {
        return "Toggle a node's %% unique name";
    }
    String description() const override {
        return "enable=true enables unique_name_in_owner on the node (prepends %% to the node name), "
               "false disables it. Unique names can be accessed directly via %%Name paths in scripts. "
               "All changes are undoable.";
    }
    Dictionary input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = "Node path";
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = "true = enable unique name, false = disable";
            p["default"] = true;
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
        bool enable = args_bool(ctx.args, "enable", true);
        Node *node = resolve_node(ctx.root, node_path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                "Node not found: " + node_path);
        }
        bool old = node->is_unique_name_in_owner();
        if (old == enable) {
            Dictionary data;
            data["node"] = relative_path(ctx.root, node);
            data["unique_name"] = enable;
            data["changed"] = false;
            return ToolResult::ok(data);
        }
        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Toggle Unique Name",
                              godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "set_unique_name_in_owner", enable);
            ur->add_undo_method(node, "set_unique_name_in_owner", old);
            ur->commit_action();
        } else {
            node->set_unique_name_in_owner(enable);
        }
        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["unique_name"] = enable;
        data["changed"] = true;
        return ToolResult::ok(data);
    }
};

}  // namespace godot_mcp
