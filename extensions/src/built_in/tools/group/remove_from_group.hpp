#pragma once

#include "built_in/tool_base.hpp"
#include "built_in/cmd_utils.hpp"

#include <godot_cpp/classes/editor_interface.hpp>

namespace godot_mcp {

class RemoveFromGroupTool : public ITool {
public:
    String name() const override { return "remove_from_group"; }
    String category() const override { return "node_tools/group"; }
    String brief() const override {
        return String("Remove a node from a group");
    }
    String description() const override {
        return String("Removes a node from a scene group. Returns an error if the node is not in the group.");
    }
    Dictionary build_input_schema() const override {
        Dictionary props;
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Node path (empty = root node of current edited scene)");
            props["node_path"] = p;
        }
        {
            Dictionary p;
            p["type"] = "string";
            p["description"] = String("Group name");
            props["group_name"] = p;
        }
        {
            Dictionary p;
            p["type"] = "boolean";
            p["description"] = String("Persist across scenes (default: true)");
            p["default"] = true;
            props["persistent"] = p;
        }
        Dictionary s;
        s["type"] = "object";
        s["properties"] = props;
        s["required"] = Array::make("node_path", "group_name");
        return s;
    }
    bool needs_scene() const override { return true; }
    bool needs_node() const override { return false; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        String path = args_string(ctx.args, "node_path", "");
        String group_name = args_string(ctx.args, "group_name");

        if (group_name.is_empty()) {
            return ToolResult::err("MISSING_ARG", String("group_name cannot be empty"));
        }

        Node *node = resolve_node(ctx.root, path);
        if (!node) {
            return ToolResult::err("NODE_NOT_FOUND",
                String("Node not found: ") + path);
        }

        if (!node->is_in_group(group_name)) {
            return ToolResult::err("NOT_IN_GROUP",
                String("Node not in group: ") + group_name);
        }

        bool persistent = args_bool(ctx.args, "persistent", true);

        godot::EditorUndoRedoManager *ur = get_undo_redo();
        if (ur) {
            ur->create_action("MCP: Remove from group", godot::UndoRedo::MERGE_DISABLE, ctx.root);
            ur->add_do_method(node, "remove_from_group", group_name);
            ur->add_undo_method(node, "add_to_group", group_name, persistent);
            ur->commit_action();
        } else {
            node->remove_from_group(group_name);
        }

        Dictionary data;
        data["node"] = relative_path(ctx.root, node);
        data["group"] = group_name;
        return ToolResult::ok(data);
    }
};

} // namespace godot_mcp
